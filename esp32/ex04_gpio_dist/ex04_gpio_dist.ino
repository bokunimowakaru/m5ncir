/*******************************************************************************
Example 05: NCIR MLX90614 Human Face Distance Meter

・非接触温度センサ の読み値から顔までの距離を求めます(測距センサ不要)。
・距離が40cm以下になった時に、検出音を鳴らし、LED緑を点灯します。
・手のひらなどをセンサに近づけた時に、ピンポン音を鳴らし、LED赤を点灯します。
　（屋外などで掌が冷えているときは検知できません）

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
#define LED_RED_PIN   16                        // 赤色LEDのIOポート番号
#define LED_GREEN_PIN 17                        // 緑色LEDのIOポート番号
#define BUZZER_PIN    25                        // IO 25にスピーカを接続
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角

float Dist = 200;                               // 測定対象までの距離(mm)
float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
int PIR_prev = 0;                               // 人体検知状態の前回の値
int PING_prev = 0;                              // 非接触ボタン状態の前回の値

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    beepSetup(BUZZER_PIN);                      // ブザー用するPWM制御部の初期化
    pinMode(LED_RED_PIN, OUTPUT);               // GPIO 16 を赤色LED用に設定
    pinMode(LED_GREEN_PIN, OUTPUT);             // GPIO 17 を緑色LED用に設定
    Wire.begin();                               // I2Cを初期化
    Serial.print("Example 04: Distance Meter [LED]"); // タイトル表示
}

void loop(){                                    // 繰り返し実行する関数
    delay(500);                                 // 0.5秒（500ms）の待ち時間処理
    float Tenv= getTemp(6);                     // センサの環境温度を取得
    float Tsen= getTemp();                      // センサの測定温度を取得
    if(Tenv < -20. || Tsen < -20.) return;      // -20℃未満のときは中断

    float Ssen = pow(Dist * tan(FOV / 360. * PI), 2.) * PI;
    float Tobj = Tenv + TempOfsAra + (Tsen - Tenv) * sqrt(Ssen / Sobj);
    float cSsen = Sobj * pow((36. - Tenv - TempOfsAra) / (Tsen - Tenv), 2.);
    Dist = sqrt(cSsen / PI) / tan(FOV / 360. * PI);

    if(Dist > 400){                             // 400mm超のとき
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        digitalWrite(LED_GREEN_PIN, LOW);       // LED緑を消灯
        PIR_prev = 0;                           // 人感センサ非検知状態に設定
        PING_prev = 0;                          // 非接触ボタンを開放状態に設定
    }else if(PIR_prev == 0){                    // 人感センサ非検知状態のとき
        digitalWrite(LED_GREEN_PIN, HIGH);      // LED緑を点灯
        beep_alert(1);                          // 検知音を鳴らす
        PIR_prev = 1;                           // 人感センサ検知状態に設定
    }else if(Dist <= 100. && PING_prev == 0){   // 100mm以下のとき
        digitalWrite(LED_RED_PIN, HIGH);        // LED赤を点灯
        beep(1109, 600);                        // 1109Hz(ピーン)音を鳴らす
        PING_prev = 1;                          // 非接触ボタンを押下状態に設定
    }else if(Dist >= 150. && PING_prev == 1){   // 150mm以上のとき
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        beep(880, 600);                         // 880(ポン)音を鳴らす
        PING_prev = 0;                          // 非接触ボタンを開放状態に設定
    }

    Serial.printf("Tenv=%.1f ",Tenv);           // 環境温度を表示
    Serial.printf("Tsen=%.1f ",Tsen);           // 測定温度を表示
    Serial.printf("Tobj=%.1f ",Tobj);           // 物体温度を表示
    Serial.printf("Dist=%.0f cm\n",Dist / 10);  // 物体(逆算)距離を表示
}
