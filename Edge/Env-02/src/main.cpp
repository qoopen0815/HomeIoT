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
#define S_PERIOD 290 // delayする秒数
RTC_DATA_ATTR static uint8_t seq; // 送信SEQ

BLEServer *pServer;
BLEAdvertising *pAdvertising;

// Sensor
#include <Adafruit_SGP30.h>
#include <UNIT_ENV.h>
Adafruit_SGP30 sgp30;
QMP6988 qmp;
SHT3X sht30;

// Other
#include <esp_sleep.h>
#include <Wire.h>

const bool SerialEnable = false;
const bool I2CEnable = true;
const bool DisplayEnable = true;

uint16_t temp;
uint16_t humid;
uint16_t press;
uint16_t tvoc;
uint16_t eco2;

uint8_t brightness = 0x40;

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

void advData(void * pvParameters) {
  while(1) { //Keep the thread running.
    M5.dis.drawpix(0, dispColor(0, brightness, 0));
    delay(S_PERIOD * 1000);                             // S_PERIOD秒delayする
    M5.dis.drawpix(0, dispColor(0, 0, brightness));
    setAdvData(pAdvertising);                           // アドバタイジングデーターをセット
    pAdvertising->start();                              // アドバタイズ起動
    delay(T_PERIOD * 1000);                             // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                               // アドバタイズ停止
    seq++;                                              // シーケンス番号を更新
  }
}

void readSensor(void * pvParameters) {
  while(1) {
    if (sht30.get()==0) {
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
    delay(5000);
  }
}

void setup() {
  M5.begin(SerialEnable, I2CEnable, DisplayEnable);
  M5.dis.drawpix(0, dispColor(brightness, brightness, brightness));
  
  Serial.begin(115200);
  Wire.begin(26, 32); // I2Cを初期化する

  BLEDevice::init("Env-02");                  // デバイスを初期化
  pServer = BLEDevice::createServer();        // サーバーを生成
  pAdvertising = pServer->getAdvertising();   // アドバタイズオブジェクトを取得

  delay(3000);

  qmp.init();
  if (! sgp30.begin()){
      if (Serial) Serial.println("SGP30 not found :(");
      M5.dis.drawpix(0, dispColor(brightness, 0, 0)); // Red: Error
      while (1);
  }

  // Creat Task1.  创建线程1
  xTaskCreatePinnedToCore(
                  advData,     //Function to implement the task.  线程对应函数名称(不能有返回值)
                  "task1",   //线程名称
                  4096,      // The size of the task stack specified as the number of * bytes.任务堆栈的大小(字节)
                  NULL,      // Pointer that will be used as the parameter for the task * being created.  创建作为任务输入参数的指针
                  1,         // Priority of the task.  任务的优先级
                  NULL,      // Task handler.  任务句柄
                  0);        // Core where the task should run.  将任务挂载到指定内核

  // Task 2
  xTaskCreatePinnedToCore(
                  readSensor,
                  "task2",
                  4096,
                  NULL,
                  2,
                  NULL,
                  0);
}

void loop() {
}
