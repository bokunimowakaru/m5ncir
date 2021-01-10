/*******************************************************************************
Example 04: NCIR MLX90614 & TOF Human Body Temperature Checker

・距離センサが人体を検出すると、測定を開始します。
・測定中は測定音を鳴らします。
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

NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°

TOFセンサ VL53L0X (STMicroelectronics製) に関する参考文献
    https://groups.google.com/d/msg/diyrovers/lc7NUZYuJOg/ICPrYNJGBgAJ
*******************************************************************************/

#include <Wire.h>                               // I2C通信用ライブラリ
#define LED_RED_PIN   16                        // 赤色LEDのIOポート番号
#define LED_GREEN_PIN 17                        // 緑色LEDのIOポート番号
#define BUZZER_PIN    25                        // IO 25にスピーカを接続

float TempWeight = 1110.73;                     // 温度(利得)補正係数
float TempOffset = 36.5;                        // 温度(加算)補正係数
float DistOffset = 29.4771;                     // 距離補正係数
float temp_sum = 0.0;                           // 体温値の合計(平均計算用)
int temp_count = 0;                             // temp_sumの測定済サンプル数

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    beepSetup(BUZZER_PIN);                      // ブザー用するPWM制御部の初期化
    pinMode(LED_RED_PIN, OUTPUT);               // GPIO 16 を赤色LED用に設定
    pinMode(LED_GREEN_PIN, OUTPUT);             // GPIO 17 を緑色LED用に設定
    Wire.begin();                               // I2Cを初期化
    Serial.println("Example 04: Body Temperature Checker [ToF][LED]");
}

void loop(){                                    // 繰り返し実行する関数
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20.) return;                     // 20mm以下の時に再測定
    if(Dist > 400){                             // 400mm超のとき
        temp_sum = 0.0;                         // 体温の合計値を0にリセット
        temp_count = 0;                         // 測定サンプル数を0にリセット
        return;                                 // 測定処理を中断
    }
    
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断
    
    // 体温Tobj = 基準温度 + センサ温度差値 - 温度利得 ÷ 距離
    float Tobj = TempOffset + (Tsen - Tenv) - TempWeight / (Dist + DistOffset);
    if(Tobj < 0. || Tobj > 99.) return;         // 0℃未満/99℃超過時は戻る
    temp_sum += Tobj;                           // 変数temp_sumに体温を加算
    temp_count++;                               // 測定済サンプル数に1を加算
    float temp_avr = temp_sum / (float)temp_count;  // 体温の平均値を算出

    if(temp_count % 5 == 0){
        Serial.printf("ToF=%.0fcm ",Dist/10);   // 測距結果を表示
        Serial.printf("Te=%.1f ",Tenv);         // 環境温度を表示
        Serial.printf("Ts=%.1f ",Tsen);         // 測定温度を表示
        Serial.printf("To=%.1f ",Tobj);         // 物体温度を表示
        Serial.printf("Tavr=%.1f\n",temp_avr);  // 平均温度を表示
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        digitalWrite(LED_GREEN_PIN, LOW);       // LED緑を消灯
        beep(1047);                             // 1047Hzのビープ音(測定中)
    }
    if(temp_count % 20 != 0) return;            // 剰余が0以外のときに先頭へ
    
    if(temp_avr >= 37.5){                       // 37.5℃以上のとき(発熱検知)
        digitalWrite(LED_RED_PIN, HIGH);        // LED赤を点灯
        digitalWrite(LED_GREEN_PIN, LOW);       // LED緑を消灯
        beep_alert(3);                          // アラート音を3回、鳴らす
    }else if(temp_avr < 35.0){                  // 35.0℃未満のとき(再測定)
        temp_sum = Tobj;                        // 最後の測定結果のみを代入
        temp_count = 1;                         // 測定済サンプル数を1に
    }else if(temp_avr + .2 < Tobj || temp_avr - .2 > Tobj){ // 最新値が遠い時
        temp_sum = Tobj;                        // 最後の測定結果のみを代入
        temp_count = 1;                         // 測定済サンプル数を1に
    }else{
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        digitalWrite(LED_GREEN_PIN, HIGH);      // LED緑を点灯
        beep_chime();                           // ピンポン音を鳴らす
        temp_sum = 0.0;                         // 体温の合計値を0にリセット
        temp_count = 0;                         // 測定サンプル数を0にリセット
        delay(3000);                            // 3秒間、待機する
    }
}
