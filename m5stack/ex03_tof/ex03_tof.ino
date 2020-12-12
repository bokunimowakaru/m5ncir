/*******************************************************************************
Example 03: NCIR MLX90614 & TOF Human Body Temperature Meter for M5Stack

・非接触温度センサ の読み値を体温に変換し、アナログ・メータ表示します。
・測距センサを使って顔までの距離を測定し、1次変換式により体温を算出します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

・対応する測距センサ：
　M5Stack Time-of-Flight Distance Ranging Sensor Unit
　STMicroelectronics VL53L0X; Time-of-Flight ranging sensor

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

TOFセンサ VL53L0X (STMicroelectronics製) に関する参考文献
    https://groups.google.com/d/msg/diyrovers/lc7NUZYuJOg/ICPrYNJGBgAJ
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリ
#include <Wire.h>                               // I2C通信用ライブラリ
float TempWeight = 1110.73;                     // 温度(利得)補正係数
float TempOffset = 36.0;                        // 温度(加算)補正係数
float DistOffset = 29.4771;                     // 距離補正係数
int lcd_row = 22;                               // 液晶画面上の行数保持用の変数

float getTemp(byte reg = 0x7){
    int16_t val = 0xFFFF;                       // 変数valを定義
    Wire.beginTransmission(0x5A);               // MLX90614(0x5A)との通信を開始
    Wire.write(reg);                            // レジスタ番号を指定
    if(Wire.endTransmission(false)==0){         // MLX90614(0x5A)との通信を継続
        Wire.requestFrom(0x5A, 2);              // 2バイトのデータを要求
        if(Wire.available() >= 2){              // 2バイト以上を受信
            val = (int16_t)Wire.read();         // 1バイト目を変数tempの下位へ
            val |= ((int16_t)Wire.read()) << 8; // 2バイト目を変数tempの上位へ
        }
    }
    return (float)val * 0.02 - 273.15;          // 温度に変換して応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC","Face Prop",30,40);  // メータのレンジおよび表示設定
    M5.Lcd.print("Example 03: Body Temperature Meter [ToF]"); // タイトル
}

void loop(){                                    // 繰り返し実行する関数
    delay(100);                                 // 0.1秒（100ms）の待ち時間処理
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20. || Dist > 400) return;       // 20mm以下/400mm超のときに戻る
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断
    // 体温計算 係数換算方式
    // 体温Tobj = 基準温度 + センサ温度差値 - 温度利得 ÷ 距離
    float Tobj = TempOffset + (Tsen - Tenv) - TempWeight / (Dist + DistOffset);
//  Serial.printf("ToF=%.1fcm, ",Dist/10);      // 測距結果を出力
//  Serial.printf("Te=%.2f, ",Tenv);            // 環境温度を出力
//  Serial.printf("Ts=%.2f, ",Tsen);            // 測定温度を出力
//  Serial.printf("To=%.2f\n",Tobj);            // 物体温度を出力
    if(Tobj < 0. || Tobj > 99.) return;         // 0℃未満/99℃超過時は戻る
    M5.Lcd.setCursor(0,lcd_row * 8);            // 液晶描画位置をlcd_row行目に
    M5.Lcd.printf("ToF=%.0fcm ",Dist/10);       // 測距結果を表示
    M5.Lcd.printf("Te=%.1f ",Tenv);             // 環境温度を表示
    M5.Lcd.printf("Ts=%.1f ",Tsen);             // 測定温度を表示
    M5.Lcd.printf("To=%.1f ",Tobj);             // 物体温度を表示
    analogMeterNeedle(Tobj);                    // 温度値をメータ表示
    lcd_row++;                                  // 行数に1を加算する
    if(lcd_row > 29) lcd_row = 22;              // 最下行まで来たら先頭行へ
    M5.Lcd.fillRect(0, lcd_row * 8, 320, 8, 0); // 描画位置の文字を消去(0=黒)
}
