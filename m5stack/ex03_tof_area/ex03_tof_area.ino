/*******************************************************************************
Example 03: NCIR MLX90614 & TOF Human Body Temperature Meter for M5Stack

・非接触温度センサ の読み値を体温に変換し、アナログ・メータ表示します。
・測距センサを使って顔までの距離を測定し、顔の面積換算により体温を算出します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

・対応する測距センサ：
　M5Stack Time-of-Flight Distance Ranging Sensor Unit
　STMicroelectronics VL53L0X; Time-of-Flight ranging sensor

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
【ご注意】本ソフトウェアはセンサを使った学習用・実験用のサンプルプログラムです。
・本ソフトウェアで測定、表示する値は体温の目安値です。
・このまま医療やウィルス等の感染防止対策用に使用することは出来ません。
・変換式は、特定の条件下で算出しており、一例として以下の条件を考慮していません。
　- センサの個体差（製造ばらつき）
　- 被験者の個人差（顔の大きさ・形状、髪の量、眼鏡の有無など）
　- 室温による顔の表面温度差（体温と室温との差により、顔の表面温度が変化する）
　- 基準体温36.0℃としていることによる検出体温37.5℃の誤差の増大※

※37.5℃を検出するのであれば、本来は体温37.5℃の人体で近似式を求めるべきですが、
　本ソフトウェアは学習用・実験用につき、約36℃の人体を使用して作成しました。
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
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角
float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
int lcd_row = 22;                               // 液晶画面上の行数保持用の変数

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC","Face Area",30,40);  // メータのレンジおよび表示設定
    M5.Lcd.print("Example 03: Body Temperature Meter [ToF][Area]"); // タイトル
}

void loop(){                                    // 繰り返し実行する関数
    delay(100);                                 // 0.1秒（100ms）の待ち時間処理
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20. || Dist > 400) return;       // 20mm以下/400mm超のときに戻る
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断
    
    // 面積換算方式
    // 体温Tobj = 環境温度 + センサ温度差値×√(センサ測定面積÷測定対象面積)
    float Ssen = pow(Dist * tan(FOV / 360. * PI), 2.) * PI;  // センサ測定面積
    float Tobj = Tenv + TempOfsAra + (Tsen - Tenv) * sqrt(Ssen / Sobj);
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
