# -*- coding: utf-8 -*-
import json
import os
import sys
import struct

from bluepy.btle import DefaultDelegate, Scanner, BTLEException
from elasticsearch_handler import ElasticsearchHandler
from datetime import datetime
from pytz import timezone

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
        self.lastseq = None
        self.lasttime = datetime.fromtimestamp(0)
        self.es_handler = ElasticsearchHandler

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev or isNewData:
            for (adtype, desc, value) in dev.getScanData():
                if desc == 'Manufacturer' and value[0:4] == 'ffff':
                    __delta = datetime.now() - self.lasttime
                    if value[4:6] != self.lastseq and __delta.total_seconds() > 11:
                        self.lastseq = value[4:6]
                        self.lasttime = datetime.now()
                        (temp, humid, press, tvoc, eco2) = struct.unpack('<hhhhh', bytes.fromhex(value[6:])) # hは2Byte整数（４つ取り出す）
                        data = {"@timestamp": timezone('Asia/Tokyo').localize(self.lasttime),
                                "Temp"  : float(temp/100), "Humid" : float(humid/100), 
                                "Press" : int(press), "TVOC" : float(tvoc), "eCO2" : float(eco2)}
                        self.es_handler.sendIndex(data)
                        print('温度= {0} 度、 湿度= {1} %、 気圧 = {2} hPa、 TVOC = {3} ppb、 eCO2 = {4} ppm'.format(temp/100, humid/100, press, tvoc, eco2))

if __name__ == "__main__":
    path = {'base': None, 'setting': None, 'mapping': None}
    path['base']    = os.path.dirname(os.path.abspath(__file__))
    path['setting'] = os.path.normpath(os.path.join(path['base'], '../configs/env02/setting.json'))
    path['mapping'] = os.path.normpath(os.path.join(path['base'], '../configs/env02/mapping.json'))
    
    handler = ElasticsearchHandler( host="localhost", port=9200,
                                    index="env02", doctype="Env-02",
                                    setting=json.load(open(path['setting'], 'r')),
                                    mapping=json.load(open(path['mapping'], 'r')) )
    delegate = ScanDelegate()
    delegate.es_handler = handler
    scanner = Scanner().withDelegate(delegate)


    print(handler._index + timezone('Asia/Tokyo').localize(datetime.now()).strftime('-%Y.%m.%d'))

    print("Success Init")

    while True:
        try:
            scanner.scan(5.0)
        except BTLEException:
            ex, ms, tb = sys.exc_info()
            print('BLE exception '+str(type(ms)) + ' at ' + sys._getframe().f_code.co_name)
