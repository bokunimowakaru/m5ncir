/*******************************************************************************
Example 01: NCIR MLX90614 Temperature Meter for M5Stack

・非接触温度センサ の読み値をアナログ・メータ表示します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
【参考文献】

Arduino IDE 開発環境イントール方法：
    https://github.com/m5stack/M5Stack/blob/master/docs/getting_started_ja.md
    https://docs.m5stack.com/#/en/related_documents/Arduino_IDE

M5Stack Arduino Library API 情報：
    https://docs.m5stack.com/#/ja/api
    https://docs.m5stack.com/#/en/arduino/arduino_api
    
NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリ
#include <Wire.h>                               // I2C通信用ライブラリ

float getTemp(byte reg = 0x7){
    int16_t val = 0xFFFF;                       // 変数valを定義
    Wire.beginTransmission(0x5A);               // MLX90614とのI2C通信を開始
    Wire.write(reg);                            // レジスタ番号を指定（送信）
    if(Wire.endTransmission(false) == 0){       // 送信を終了（接続は継続）
        Wire.requestFrom(0x5A, 2);              // 2バイトのデータを要求
        val = (int16_t)Wire.read();             // 1バイト目を変数tempの下位へ
        val |= ((int16_t)Wire.read()) << 8;     // 2バイト目を変数tempの上位へ
    }
    Wire.endTransmission();                     // I2C通信の切断
    return (float)val * 0.02 - 273.15;          // 温度に変換して応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC", "NCIR", 0, 40);     // メータのレンジおよび表示設定
    M5.Lcd.print("Example 01: Temperature Meter"); // タイトル表示
}

void loop(){                                    // 繰り返し実行する関数
    delay(100);                                 // 0.1秒（100ms）の待ち時間処理
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tsen < -20.) return;                     // -20℃未満のときは中断
//  Serial.printf("Tsen=%.2f\n",Tsen);          // 温度値をシリアル表示
    analogMeterNeedle(Tsen);                    // 温度値をメータ表示
}
