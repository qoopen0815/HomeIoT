// M5 board library
#ifdef ARDUINO_M5Stack_Core_ESP32
#include <Arduino.h>
#include <M5Stack.h>
#endif
#ifdef ARDUINO_M5Stick_C
#include <Arduino.h>
#include <M5StickC.h>
#endif

// Bluetooth Low Energy
#include <BLEDevice.h> 
#include <BLEServer.h>
#include <BLEUtils.h>
#define T_PERIOD 10  // アドバタイジングパケットを送る秒数
#define S_PERIOD 290 // Deep Sleepする秒数
RTC_DATA_ATTR static uint8_t seq; // 送信SEQ

// Sensor
#include <UNIT_ENV.h>
QMP6988 qmp;
SHT3X sht30;

// Other
#include <esp_sleep.h>
#include <Wire.h>

uint16_t temp;
uint16_t humid;
uint16_t press;
uint16_t vbat;

void setAdvData(BLEAdvertising *pAdvertising) {
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x0c;   // 長さ
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
    strServiceData += (char)(vbat & 0xff);         // 電池電圧の下位バイト
    strServiceData += (char)((vbat >> 8) & 0xff);  // 電池電圧の上位バイト

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
    M5.begin();
    M5.Axp.ScreenBreath(10);    // 画面の輝度を下げる
    M5.Lcd.setRotation(3);      // 左を上にする
    M5.Lcd.setTextSize(1);      // 文字サイズを2にする
    M5.Lcd.setCursor(0, 0, 2);
    M5.Lcd.fillScreen(BLACK);   // 背景を黒にする

    Wire.begin(0, 26);          // I2Cを初期化する(hat用)
    qmp.init();

    if (sht30.get()==0)
    {
        temp  = (uint16_t)(sht30.cTemp * 100);
        humid = (uint16_t)(sht30.humidity * 100);
    }
    press = (uint16_t)(qmp.calcPressure() / 100);
    vbat  = (uint16_t)(M5.Axp.GetVbatData() * 1.1 / 10);
    
    M5.Lcd.setCursor(0, 0, 1); 
    M5.Lcd.printf("Temp: %2.2f 'C\r\n",  (float)temp / 100);
    M5.Lcd.printf("Humid: %2.2f %%\r\n",  (float)humid / 100);
    M5.Lcd.printf("Press: %2.2f hPa\r\n", (float)press);
    M5.Lcd.printf("Battery: %4.2f V\r\n",   (float)vbat / 100);

    BLEDevice::init("Env-01");                  // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成

    BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                          // アドバタイジングデーターをセット

    pAdvertising->start();                             // アドバタイズ起動
    delay(T_PERIOD * 1000);                            // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                              // アドバタイズ停止

    seq++;                                             // シーケンス番号を更新
    delay(10);
    esp_deep_sleep(1000000LL * S_PERIOD);              // S_PERIOD秒Deep Sleepする
}

void loop() {
}
