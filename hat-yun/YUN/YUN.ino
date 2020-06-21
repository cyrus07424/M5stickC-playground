// please install Adafruit_BMP280 lib first
// Adafruit_BMP280 lib in Sketch->Includ Library->Library Manager

#include "M5StickC.h"
#include "SHT20.h"
#include <Adafruit_BMP280.h>
#include "yunBoard.h"
#include <math.h>
#include "display.h"
#include "Ambient.h"

// 温度・湿度センサ(SHT20)
SHT20 sht20;

// 気圧センサ(BMP280)
Adafruit_BMP280 bmp;

// 明るさ(別ファイルでextern変数として使用するためグローバル変数として定義)
uint16_t light;

// LEDの値
extern uint8_t lightR;
extern uint8_t lightG;
extern uint8_t lightB;

// LEDの点灯フラグ
uint8_t light_flag = 0;

// Wifiクライアント
WiFiClient wifiClient;

// Wifiステータス
int wifiStatus = WL_IDLE_STATUS;

// Wifi SSID
const char* ssid = "CHANGEME";

// Wifi パスワード
const char* password = "CHANGEME";

// Ambient
Ambient ambient;

// Ambient チャネルID
unsigned int channelId = CHANGEME;

// Ambient ライトキー
const char* writeKey = "CHANGEME";

// 次回の実行時間(画面描画)
uint32_t update_time = 0;

// 次回の実行時間(Ambient)
uint32_t ambient_update_time = 0;

void setup() {
  // M5ライブラリの初期化
  M5.begin();
  Wire.begin(0, 26, 100000);

  // LCDの方向を設定
  M5.Lcd.setRotation(1);

  // LCDのフォントサイズを設定
  M5.Lcd.setTextSize(2);

  // 気圧センサのチェック
  if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
  }

  // 気圧センサの初期設定
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_1000); /* Standby time. */

  // LEDによって温度が変化するためLEDを消灯
  led_off();

  // チャネルIDとライトキーを指定してAmbientの初期化
  ambient.begin(channelId, writeKey, &wifiClient);
}

void loop() {
  // 次回の実行時間(画面描画)に達している場合
  if (millis() > update_time) {
    // 次回の実行時間(画面描画)を設定(1秒毎に実行)
    update_time = millis() + 1000;

    // 温度を取得
    float tmp = sht20.read_temperature();

    // 湿度を取得
    float hum = sht20.read_humidity();

    // 気圧を取得
    float pressure = bmp.readPressure();

    // 明るさを取得
    light = light_get();

    // 画面描画
    M5.Lcd.setCursor(3, 3);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.print("YUN");
    M5.Lcd.setCursor(3, 25);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.printf("%.3f", tmp);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.printf("C");
    M5.Lcd.setCursor(3, 25 + 20);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.printf("%.3f", hum);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.printf("%%");
    M5.Lcd.setCursor(3, 25 + 20 + 20);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.printf("%.2f", pressure / 100);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.printf("hPa");

    // 明るさによって画面を描画
    display_light();

    // 次回の実行時間(Ambient)に達している場合
    if (ambient_update_time < millis()) {
      // 次回の実行時間(Ambient)を設定(1分毎に実行)
      ambient_update_time = millis() + (60 * 1000);

      // Ambientにデータを送信
      sendAmbient(tmp, hum, pressure, light);
    }
  }

  // 更新
  M5.update();

  // Aボタンが押されていた場合
  if (M5.BtnA.wasPressed()) {
    // 再起動
    esp_restart();
  }

  // ウエイト
  delay(10);
}

/**
    Ambientにデータを送信.
*/
void sendAmbient(float tmp, float hum, float pressure, uint16_t light) {
  // Wifi接続確認
  int count = 0;
  while (wifiStatus != WL_CONNECTED && count < 10) {
    count++;
    Serial.println("connecting Wifi...");

    // Wifi接続
    wifiStatus = WiFi.begin(ssid, password);
    delay(1000);
  }

  // Ambientにデータを送信.
  ambient.set(1, tmp);
  ambient.set(2, hum);
  ambient.set(3, light);
  ambient.set(4, pressure);
  if (ambient.send()) {
    Serial.println("Ambient::send() success");
  } else {
    Serial.println("Ambient::send() failure");
  }

  // debug
  M5.Lcd.setCursor(3, 3);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.print("!");
}
