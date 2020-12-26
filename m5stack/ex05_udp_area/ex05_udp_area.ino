/*******************************************************************************
Example 05: NCIR MLX90614 & TOF Human Body Temperature Checker for M5Stack

・距離センサが人体を検出すると、測定を開始します。
・37.5℃以上だった場合、警告音を鳴らします。
・35.0～37.5℃だった場合、ピンポン音を鳴らします。
・LAN内にUDPブロードキャストで通知します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

・対応する測距センサ：
　M5Stack Time-of-Flight Distance Ranging Sensor Unit
　STMicroelectronics VL53L0X; Time-of-Flight ranging sensor

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
・Raspberry Piでの受信例：
    $ cd
    $ git clone https://bokunimo.net/git/m5ncir.git
    $ cd m5ncir/tools
    $ ./udp_logger.py
    UDP Logger (usage: ./udp_logger.py port)
    Listening UDP port 1024 ...
    2020/12/24 21:36, 192.168.1.15, pir_s_5,1, 0, 33.6      (1)検知
    2020/12/24 21:36, 192.168.1.15, pir_s_5,1, 1, 34.8      (2)測定中
    2020/12/24 21:36, 192.168.1.15, pir_s_5,1, 1, 36.0
    2020/12/24 21:36, 192.168.1.15, pir_s_5,1, 1, 36.3
    2020/12/24 21:36, 192.168.1.15, pir_s_5,1, 1, 36.5
    2020/12/24 21:36, 192.168.1.15, pir_s_5,0, 1, 36.5      (3)測定完了
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
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <WiFiUdp.h>                            // UDP通信を行うライブラリ
#define SSID "iot-core-esp32"                   // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define PORT 1024                               // 送信のポート番号
#define DEVICE "pir_s_5,"                       // デバイス名(5字+"_"+番号+",")
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角

float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
int LCD_row = 22;                               // 液晶画面上の行数保持用の変数
int PIR_prev = 0;                               // 人体検知状態の前回の値
float temp_sum = 0.0;                           // 体温値の合計(平均計算用)
int temp_count = 0;                             // temp_sumの測定済サンプル数
IPAddress IP_BROAD;                             // ブロードキャストIPアドレス

void sendUdp(String dev, String S){
    WiFiUDP udp;                                // UDP通信用のインスタンスを定義
    udp.beginPacket(IP_BROAD, PORT);            // UDP送信先を設定
    udp.println(dev + S);                       // 変数devとSを結合してUDP送信
    udp.endPacket();                            // UDP送信の終了(実際に送信する)
    Serial.println("udp://" + IP_BROAD.toString() + ":" + PORT + " " + dev + S);
    M5.Lcd.print(S);                            // LCDに送信内容を表示
    delay(100);                                 // 送信待ち時間
}

void sendUdp_Pir(int pir, float temp){
    String S = String(pir);                     // 変数Sに人体検知状態を代入
    S +=  ", " + String(PIR_prev);              // 前回値を追加
    S +=  ", " + String(temp, 1);               // 体温を追加
    sendUdp(DEVICE, S);                         // sendUdpを呼び出し
    PIR_prev = pir;                             // 今回の値を前回値として更新
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC","Face Area",30,40);  // メータのレンジおよび表示設定
    M5.Lcd.println("Example 05: Body Temperature Checker [ToF][UDP]");
    delay(500);                                 // 電源安定待ち時間処理0.5秒
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理
        M5.Lcd.print('.');                      // 進捗表示
    }
    IP_BROAD = WiFi.localIP();                  // IPアドレスを取得
    IP_BROAD[3] = 255;                          // ブロードキャストアドレスに
}

void loop(){                                    // 繰り返し実行する関数
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20.) return;                     // 20mm以下の時に再測定
    if(Dist > 400){                             // 400mm超のとき
        if(PIR_prev == 1 && temp_count > 0){    // 人体検知中で測定値がある時
            sendUdp_Pir(0,temp_sum/(float)temp_count); // 体温の平均値をUDP送信
        }
        temp_sum = 0.0;                         // 体温の合計値を0にリセット
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
    float temp_avr = temp_sum / (float)temp_count;  // 体温の平均値を算出
    
    if(temp_count % 5 == 0){
        M5.Lcd.setCursor(0,LCD_row * 8);        // 液晶描画位置をLCD_row行目に
        M5.Lcd.printf("ToF=%.0fcm ",Dist/10);   // 測距結果を表示
        M5.Lcd.printf("Te=%.1f ",Tenv);         // 環境温度を表示
        M5.Lcd.printf("Ts=%.1f ",Tsen);         // 測定温度を表示
        M5.Lcd.printf("To=%.1f ",Tobj);         // 物体温度を表示
        M5.Lcd.printf("Tavr=%.1f ",temp_avr);   // 平均温度を表示
        analogMeterNeedle(temp_avr);            // 温度値をメータ表示
        LCD_row++;                              // 行数に1を加算する
        if(LCD_row > 29) LCD_row = 22;          // 最下行まで来たら先頭行へ
        M5.Lcd.fillRect(0,LCD_row * 8,320,8,0); // 描画位置の文字を消去(0=黒)
        sendUdp_Pir(1, temp_avr);               // 体温の平均値をUDP送信
        beep(1047);                             // 1047Hzのビープ音(測定中)
    }
    if(temp_count % 20 != 0) return;            // 剰余が0以外のときに先頭へ
    
    if(temp_avr >= 37.5){                       // 37.5℃以上のとき(発熱検知)
        beep_alert(3);                          // アラート音を3回、鳴らす
    }else if(temp_avr < 35.0){                  // 35.0℃未満のとき(再測定)
        temp_sum = Tobj;                        // 最後の測定結果のみを代入
        temp_count = 1;                         // 測定済サンプル数を1に
    }else{
        sendUdp_Pir(0, temp_avr);               // 体温の平均値をUDP送信
        beep_chime();                           // ピンポン音を鳴らす
        temp_sum = 0.0;                         // 体温の合計値を0にリセット
        temp_count = 0;                         // 測定サンプル数を0にリセット
        delay(3000);                            // 3秒間、待機する
    }
}
