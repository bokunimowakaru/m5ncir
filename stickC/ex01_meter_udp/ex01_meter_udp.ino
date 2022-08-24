/*******************************************************************************
Example 01: NCIR MLX90614 Temperature Meter for M5StickC

・非接触温度センサ の読み値をアナログ・メータ表示します。
・読み値をUDPで送信します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2020-2022 Wataru KUNINO
********************************************************************************
【参考文献】

Arduino IDE 開発環境イントール方法：
    https://github.com/m5stack/M5Stack/blob/master/docs/getting_started_ja.md
    https://docs.m5stack.com/#/en/related_documents/Arduino_IDE

M5StickC Arduino Library API 情報：
    https://docs.m5stack.com/en/api/stickc/system_m5stickc
    
NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°
*******************************************************************************/

#include <M5StickC.h>                           // M5StickC用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#include <Wire.h>                               // I2C通信用ライブラリ
#include "esp_sleep.h"                          // ESP32用Deep Sleep ライブラリ

#define SSID "iot-core-esp32"                   // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define DEVICE "temp._5,"                       // デバイス名(5字+"_"+番号+",")
#define OFFSET +2.0                             // 測定値の補正
IPAddress UDPTO_IP = {255,255,255,255};         // UDP宛先 IPアドレス

float getTemp(byte reg = 0x7){
    int16_t val = 0xFFFF;                       // 変数valを定義
    Wire.beginTransmission(0x5A);               // MLX90614とのI2C通信を開始
    Wire.write(reg);                            // レジスタ番号を指定（送信）
    if(Wire.endTransmission(false) == 0){       // 送信を終了（接続は継続）
        Wire.requestFrom(0x5A, 2);              // 2バイトのデータを要求
        val = (int16_t)Wire.read();             // 1バイト目を変数tempの下位へ
        val |= ((int16_t)Wire.read()) << 8;     // 2バイト目を変数tempの上位へ
    }
    return (float)val * 0.02 - 273.15;          // 温度に変換して応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin(0,26);                           // I2Cを初期化
    M5.Axp.ScreenBreath(7+2);                   // LCDの輝度を2に設定
    M5.Lcd.setRotation(1);                      // LCDを横向き表示に設定
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    analogMeterInit("degC", "NCIR", 36, 40);    // メータのレンジおよび表示設定
}

void loop(){                                    // 繰り返し実行する関数
    M5.update();                                // ボタン状態の取得
    float Tsen = 0.;
    for(int i=0; i<10; i++){
        Tsen += getTemp() + OFFSET;             // センサの測定温度を取得
        delay(10);
    }
    Tsen /= 10;

    M5.Axp.ScreenBreath(7 + 2 - (millis() > 3000)); // 起動後3秒以上でLCDを暗く
    analogMeterNeedle(Tsen,5);                  // 照度に応じてメータ針を設定
    M5.Lcd.setTextColor(BLACK,WHITE);           // 文字の色を黒、背景色を白に
    M5.Lcd.setCursor(0,0);                      // 表示位置を原点(左上)に設定
    if(WiFi.status() != WL_CONNECTED ){         // Wi-Fi未接続のとき
        M5.Lcd.printf("(%d)",WiFi.status());    // Wi-Fi状態番号を表示
        if(millis() > 30000) sleep();           // 30秒超過でスリープ
        return;                                 // loop関数を繰り返す
    }

    M5.Lcd.println(WiFi.localIP());             // 本機のアドレスをシリアル出力
    if(!M5.BtnA.read()){                        // ボタン非押下 
        if(millis() > 30000) sleep();           // 起動後30秒超過
        if(millis() < 3000) return;             // 起動後3秒以下
        if(Tsen <= 35.0) return;                // 35℃以下
    }
    M5.Axp.ScreenBreath(7+2);                   // LCDの輝度を2に設定
    String val_S = String(Tsen,1);
    M5.Lcd.drawCentreString(val_S,80,34,4);
    String tx_S = String(DEVICE) + val_S;       // 送信データSにデバイス名を代入
    Serial.println(tx_S);                       // 送信データSをシリアル出力表示
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(UDPTO_IP, PORT);            // UDP送信先を設定
    udp.println(tx_S);                          // 送信データSをUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    delay(6000);                                // 6秒間、表示
    sleep();
}

void sleep(){                                   // スリープ実行用の関数
    delay(100);                                 // 送信完了の待ち時間処理
    WiFi.disconnect();                          // Wi-Fiの切断
    while(M5.BtnA.read());                      // ボタン開放待ち
    M5.Axp.ScreenBreath(0);                     // LCD用バックライトの消灯
    M5.Lcd.fillScreen(BLACK);                   // LCDの消去
    M5.Axp.SetLDO2(false);                      // LCDバックライト用電源OFF
    Serial.println("Sleep...");                 // 「Sleep」をシリアル出力表示
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_37,0); // ボタン割込み設定(G37)
    esp_deep_sleep_start();                     // Deep Sleepモードへ移行
}
