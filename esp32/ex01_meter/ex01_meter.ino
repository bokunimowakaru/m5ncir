/*******************************************************************************
Example 01: NCIR MLX90614 Temperature Meter

・非接触温度センサ の読み値をシリアル出力します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
【参考文献】

NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°
*******************************************************************************/

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
    return (float)val * 0.02 - 273.15;          // 温度に変換して応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    Wire.begin();                               // I2Cを初期化
    Serial.println("Example 01: Temperature Meter"); // タイトル表示
}

void loop(){                                    // 繰り返し実行する関数
    delay(500);                                 // 0.5秒（500ms）の待ち時間処理
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tsen < -20.) return;                     // -20℃未満のときは中断
    Serial.printf("Tsen=%.2f\n",Tsen);          // 温度値をシリアル表示
}
