/*******************************************************************************
Example 05: NCIR MLX90614 Human Face Distance Meter

・非接触温度センサ の読み値から顔までの距離を求めます(測距センサ不要)。
・距離が40cm以下になった時に、検出音を鳴らします。
・手のひらなどをセンサに近づけた時に、ピンポン音を鳴らします。
　（屋外などで掌が冷えているときは検知できません）
・LAN内にUDPブロードキャストで通知します。

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
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#define SSID "iot-core-esp32"                   // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define DEVICE "pir_s_5,"                       // デバイス名(5字+"_"+番号+",")
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
IPAddress IP_BROAD;                             // ブロードキャストIPアドレス

void sendUdp(String dev, String S){
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(IP_BROAD, PORT);            // UDP送信先を設定
    udp.println(dev + S);                       // 変数devとSを結合してUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    Serial.println("udp://" + IP_BROAD.toString() + ":" + PORT + " " + dev + S);
    delay(100);                                 // 送信待ち時間
}

void sendUdp_PingPong(int in){
    if(in){
        sendUdp("Ping","");                     // PingをUDP送信
        beep(1109, 600);                        // 1109Hz(ピーン)音を鳴らす
    }else{
        sendUdp("Pong","");                     // PingをUDP送信
        beep(880, 600);                         // 880(ポン)音を鳴らす
    }
    PING_prev = in;                             // 今回の値を前回値として更新
}

void sendUdp_Pir(int pir){
    String S = String(pir);                     // 変数Sに人体検知状態を代入
    S +=  ", " + String(PIR_prev);              // 前回値を追加
    sendUdp(DEVICE, S);                         // sendUdpを呼び出し
    PIR_prev = pir;                             // 今回の値を前回値として更新
}

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    beepSetup(BUZZER_PIN);                      // ブザー用するPWM制御部の初期化
    Wire.begin();                               // I2Cを初期化
    Serial.print("Example 05: Distance Meter [UDP]"); // タイトル表示
    delay(500);                                 // 電源安定待ち時間処理0.5秒
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理
        Serial.print('.');                      // 進捗表示
    }
    IP_BROAD = WiFi.localIP();                  // IPアドレスを取得
    IP_BROAD[3] = 255;                          // ブロードキャストアドレスに
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
        if(PIR_prev == 1) sendUdp_Pir(0);       // 検知中ならPIR=OFFをUDP送信
        if(PING_prev == 1) sendUdp_PingPong(0); // 押下中ならPongをUDP送信
    }else if(PIR_prev == 0){                    // 人感センサ非検知状態のとき
        sendUdp_Pir(1);                         // PIR=ON(1)をUDP送信
        beep_alert(1);                          // 検知音を鳴らす
    }else if(Dist <= 100. && PING_prev == 0){   // 100mm以下のとき
        sendUdp_PingPong(1);                    // PingをUDP送信する
    }else if(Dist >= 150. && PING_prev == 1){   // 150mm以上のとき
        sendUdp_PingPong(0);                    // PongをUDP送信する
    }

    Serial.setCursor(0,LCD_row * 8);            // 液晶描画位置をLCD_row行目に
    Serial.printf("Tenv=%.1f ",Tenv);           // 環境温度を表示
    Serial.printf("Tsen=%.1f ",Tsen);           // 測定温度を表示
    Serial.printf("Tobj=%.1f ",Tobj);           // 物体温度を表示
    Serial.printf("Dist=%.0f cm\n",Dist / 10);  // 物体(逆算)距離を表示
}
