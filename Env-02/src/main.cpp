// M5 board library
#ifdef ARDUINO_M5Stick_C
#include <Arduino.h>
#include <M5Atom.h>
#endif

// Bluetooth Low Energy
#include <BLEDevice.h> 
#include <BLEServer.h>
#include <BLEUtils.h>
#define T_PERIOD 10  // アドバタイジングパケットを送る秒数
#define S_PERIOD 290 // Deep Sleepする秒数
RTC_DATA_ATTR static uint8_t seq; // 送信SEQ

// Sensor
#include <Adafruit_SGP30.h>
#include <UNIT_ENV.h>
Adafruit_SGP30 sgp30;
QMP6988 qmp;
SHT3X sht30;

// Other
#include <esp_sleep.h>
#include <Wire.h>

const bool LCDEnable = false;
const bool PowerEnable = false;
const bool SerialEnable = true;

uint16_t temp;
uint16_t humid;
uint16_t press;
uint16_t tvoc;
uint16_t eco2;

uint8_t brightness;

CRGB dispColor(uint8_t g, uint8_t r, uint8_t b) {
  return (CRGB)((g << 16) | (r << 8) | b);
}

void setAdvData(BLEAdvertising *pAdvertising) {
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x0e;   // 長さ
    strServiceData += (char)0xff;   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;   // Test manufacture ID low byte
    strServiceData += (char)0xff;   // Test manufacture ID high byte
    strServiceData += (char)seq;                   // シーケンス番号
    strServiceData += (char)(temp & 0xff);         // 温度の下位バイト
    strServiceData += (char)((temp >> 8) & 0xff);  // 温度の上位バイト
    strServiceData += (char)(humid & 0xff);        // 湿度の下位バイト
    strServiceData += (char)((humid >> 8) & 0xff); // 湿度の上位バイト
    strServiceData += (char)(press & 0xff);        // 気圧の下位バイト
    strServiceData += (char)((press >> 8) & 0xff); // 気圧の上位バイト
    strServiceData += (char)(tvoc & 0xff);         // TVOCの下位バイト
    strServiceData += (char)((tvoc >> 8) & 0xff);  // TVOCの上位バイト
    strServiceData += (char)(eco2 & 0xff);         // eCO2の下位バイト
    strServiceData += (char)((eco2 >> 8) & 0xff);  // eCO2の上位バイト

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
    M5.begin(LCDEnable, PowerEnable, SerialEnable);
    M5.dis.drawpix(0, dispColor(brightness, brightness, brightness));
    brightness = 0x40;
    
    Serial.begin(115200);

    Wire.begin(26, 32); // I2Cを初期化する
    qmp.init();
    
    if (! sgp30.begin()){
        if (Serial) Serial.println("SGP30 not found :(");
        M5.dis.drawpix(0, dispColor(brightness, 0, 0)); // Red: Error
        while (1);
    }
}

void loop() {
    M5.dis.drawpix(0, dispColor(0, brightness, 0));
    if (sht30.get()==0)
    {
        temp  = (uint16_t)(sht30.cTemp * 100);
        humid = (uint16_t)(sht30.humidity * 100);
    }
    press = (uint16_t)(qmp.calcPressure() / 100);
    
    if (! sgp30.IAQmeasure()) {
        if (Serial) Serial.println("SGP30 measurement failed");
        return;
    }
    tvoc = (uint16_t)(sgp30.TVOC);
    eco2 = (uint16_t)(sgp30.eCO2);
    
    Serial.printf("Temp: %2.2f 'C\r\n",  (float)temp / 100);
    Serial.printf("Humid: %2.2f %%\r\n",  (float)humid / 100);
    Serial.printf("Press: %2.2f hPa\r\n", (float)press);
    Serial.printf("TVOC: %2.2f ppb\r\n", (float)tvoc);
    Serial.printf("eCO2: %2.2f ppm\r\n", (float)eco2);

    BLEDevice::init("Env-02");                          // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();     // サーバーを生成

    BLEAdvertising *pAdvertising = pServer->getAdvertising();   // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                           // アドバタイジングデーターをセット

    pAdvertising->start();                              // アドバタイズ起動
    M5.dis.drawpix(0, dispColor(0, 0, brightness));
    delay(T_PERIOD * 1000);                             // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                               // アドバタイズ停止
    M5.dis.drawpix(0, dispColor(0, brightness, 0));

    seq++;                                              // シーケンス番号を更新
    delay(S_PERIOD * 1000);                             // S_PERIOD秒delayする
}
