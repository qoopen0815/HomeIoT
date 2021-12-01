# -*- coding: utf-8 -*-
# from _typeshed import Self
from elasticsearch import Elasticsearch
import json

class ElasticsearchHandler:
  def __init__(self, host='localhost', port=9200, index="hoge", doctype="fuga", setting=json, mapping=json):
    self._setting = setting
    self._mapping = mapping
    self._index = index
    self._doctype = doctype
    self._es = Elasticsearch([f"http://{host}:{str(port)}"])
    if self._es.ping():
      print('Elasticsearch is connected !!')
      self.createIndex()
      self._es.indices.put_mapping( index=self._index,
                                    doc_type=self._doctype, 
                                    body=self._mapping, 
                                    include_type_name=True )
    else:
      print('Failed to connect !!')
      while True:
        pass
    
  def createIndex(self):
    if not self._es.indices.exists(index=self._index):
      self._es.indices.create(index=self._index)

  def deleteIndex(self):
    if self._es.indices.exists(index=self._index):
      self._es.indices.delete(index=self._index)

  def cleanIndex(self):
    if self._es.indices.exists(index=self._index):
      self._es.indices.delete(index=self._index)
      self._es.indices.create(index=self._index)

  def sendIndex(self, data: dict):
    self._es.index(index=self._index, doc_type=self._doctype, id=data['@timestamp'], body=data)


if __name__ == "__main__":
  handler = ElasticsearchHandler( host="localhost", port="port",
                                  index="sample", doctype="sample",
                                  setting=json,
                                  mapping=json.load(open('configs/mapping.json', 'r'))
                                  )

