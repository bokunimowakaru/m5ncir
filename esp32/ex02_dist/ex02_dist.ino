/*******************************************************************************
Example 02: NCIR MLX90614 Human Face Distance Meter

・非接触温度センサ の読み値から顔までの距離を求めます。
・あらかじめ設定した距離と対象物の面積、顔温度との差分値で補正します。
・体温を36℃としたときにの顔までの距離を逆算します。

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
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角
float Dist = 200;                               // 測定対象までの距離(mm)
float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰

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
    Serial.begin(115200);                       // シリアル通信速度を設定する
    Wire.begin();                               // I2Cを初期化
    Serial.println("Example 02: Distance Meter"); // タイトル表示
}

void loop(){                                    // 繰り返し実行する関数
    delay(500);                                 // 0.5秒（500ms）の待ち時間処理
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断

    // 体温Tobj = 環境温度 + センサ温度差値×√(センサ測定面積÷測定対象面積)
    float Ssen = pow(Dist * tan(FOV / 360. * PI), 2.) * PI;  // センサ測定面積
    float Tobj = Tenv + TempOfsAra + (Tsen - Tenv) * sqrt(Ssen / Sobj);
    
    // 距離の逆算（体温36℃ の人の顔までの距離cDistを上式から逆算する）
    float cSsen = Sobj * pow((36. - Tenv - TempOfsAra) / (Tsen - Tenv), 2.);
    float cDist = sqrt(cSsen / PI) / tan(FOV / 360. * PI);
    
    Serial.printf("Tenv=%.2f, ",Tenv);                      // 環境温度を出力
    Serial.printf("Tsen=%.2f(%.0fcm2), ",Tsen, Ssen / 100); // 測定温度を出力
    Serial.printf("Tobj=%.2f(%.0fcm2), ",Tobj, Sobj / 100); // 物体温度を出力
    Serial.printf("Dist=%.2f\n",cDist / 10);    // 物体(逆算)距離を出力
}
