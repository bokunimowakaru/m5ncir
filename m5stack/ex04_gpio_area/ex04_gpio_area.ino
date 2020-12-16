/*******************************************************************************
Example 04: NCIR MLX90614 & TOF Human Body Temperature Checker for M5Stack

・距離センサが人体を検出すると、測定を開始します。
・測定中は緑色LEDが点滅するとともに、測定音を鳴らします。
・測定が完了したときに37.5℃以上だった場合、警告音を鳴らし、赤LEDを点灯します。
・35.0～37.5℃だった場合、ピンポン音を鳴らします。

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
#define LED_RED_PIN   18                        // 赤色LEDのIOポート番号
#define LED_GREEN_PIN 19                        // 緑色LEDのIOポート番号
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角

float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
int lcd_row = 22;                               // 液晶画面上の行数保持用の変数
int pir_prev = 0;                               // 人体検知状態の前回値
float temp_sum = 0.0;                           // 体温値の合計(平均計算用)
int temp_count = 0;                             // temp_sumの測定済サンプル数

void beep(int freq = 880, int t = 100){         // ビープ音を鳴らす関数
    M5.Lcd.invertDisplay(false);                // 画面を反転
    M5.Speaker.begin();                         // M5Stack用スピーカの起動
    M5.Speaker.tone(freq);                      // スピーカ出力 freq Hzの音を出力
    delay(t);                                   // 0.1秒(100ms)の待ち時間処理
    M5.Speaker.end();                           // スピーカ出力を停止する
    M5.Lcd.invertDisplay(true);                 // 画面の反転を戻す
}

void beep_chime(){                              // チャイム音を鳴らす関数
    beep(1109, 600);                            // ピーン音(1109Hz)を0.6秒再生
    delay(300);                                 // 0.3秒(300ms)の待ち時間処理
    beep(880, 100);                             // ポン音(880Hz)を0.6秒再生
}

void beep_alert(int num = 3){
    for(; num > 0 ; num--) for(int i = 2217; i > 200; i /= 2) beep(i);
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    pinMode(LED_RED_PIN, OUTPUT);               // GPIO 18 を赤色LED用に設定
    pinMode(LED_GREEN_PIN, OUTPUT);             // GPIO 19 を緑色LED用に設定
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC","Face Area",30,40);  // メータのレンジおよび表示設定
    M5.Lcd.println("Example 04: Body Temperature Checker [ToF][LED]");
}

void loop(){                                    // 繰り返し実行する関数
    delay(1);                                   // 1msの待ち時間処理
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20.) return;                     // 20mm以下の時に再測定
    if(Dist > 400){                             // 400mm超のとき
        digitalWrite(LED_GREEN_PIN, !digitalRead(LED_RED_PIN)); // 赤色LEDの反転
        temp_sum = 0;                           // 体温の合計値を0にリセット
        temp_count = 0;                         // 測定サンプル数を0にリセット
        return;                                 // 測定処理を中断
    }
    
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断
    
    // 体温Tobj = 環境温度 + センサ温度差値×√(センサ測定面積÷測定対象面積)
    float Ssen = pow(Dist * tan(FOV / 360. * PI), 2.) * PI;  // センサ測定面積
    float Tobj = Tenv + TempOfsAra + (Tsen - Tenv) * sqrt(Ssen / Sobj);
    if(Tobj < 0. || Tobj > 99.) return;         // 0℃未満/99℃超過時は戻る
    temp_sum += Tobj;                           // 変数temp_sumに体温を加算
    temp_count++;                               // 測定済サンプル数に1を加算
    
    if(temp_count % 5 == 0){
        M5.Lcd.setCursor(0,lcd_row * 8);        // 液晶描画位置をlcd_row行目に
        M5.Lcd.printf("ToF=%.0fcm ",Dist/10);   // 測距結果を表示
        M5.Lcd.printf("Te=%.1f ",Tenv);         // 環境温度を表示
        M5.Lcd.printf("Ts=%.1f ",Tsen);         // 測定温度を表示
        M5.Lcd.printf("To=%.1f ",Tobj);         // 物体温度を表示
        analogMeterNeedle(Tobj);                // 温度値をメータ表示
        lcd_row++;                              // 行数に1を加算する
        if(lcd_row > 29) lcd_row = 22;          // 最下行まで来たら先頭行へ
        M5.Lcd.fillRect(0,lcd_row * 8,320,8,0); // 描画位置の文字を消去(0=黒)
        digitalWrite(LED_GREEN_PIN, !digitalRead(LED_GREEN_PIN)); // LED緑を点滅
        beep(1047);                             // 1047Hzのビープ音(測定中)
    }
    
    float temp_avr = temp_sum / (float)temp_count;  // 体温の平均値を算出
    if((pir_prev == 1 || temp_count < 10) && temp_count < 50) return;
    
    // 実行要件(以下のどちらかを満たした時)
    //  pir_prev かつ temp_count が 10 以上のとき 
    //  または temp_count が 50 以上のとき
    if(temp_avr >= 37.5){                       // 37.5℃以上のとき
        digitalWrite(LED_RED_PIN, HIGH);        // LED赤を点灯
        beep_alert(3);                          // アラート音を3回、鳴らす
    }else if(temp_avr >= 35.0){                 // 35.0℃以上の時
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        if(temp_count >= 50){                   // 測定が完了した時
            beep_chime();                       // ピンポン音を鳴らす
            delay(5000);                        // 5秒間、待機する
        }
    }
    temp_sum = Tobj;                            // 最後の測定結果のみを代入
    temp_count = 1;                             // 測定済サンプル数を1に
}
