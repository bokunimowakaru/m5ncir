/*******************************************************************************
Example 06: NCIR MLX90614 Human Face Distance Meter

・非接触温度センサ の読み値から顔までの距離を求めます(測距センサ不要)。
・距離が40cm以下になった時に、検出音を鳴らします。
・手のひらなどをセンサに近づけた時に、ピンポン音を鳴らします。
　（屋外などで掌が冷えているときは検知できません）
・測定値をLINEアプリに通知します。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
【参考文献】

NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°

LINE Notify API Document
    https://notify-bot.line.me/doc/ja/
 
    POST https://notify-api.line.me/api/notify
        Method  POST
        Content-Type    application/x-www-form-urlencoded
        Authorization   Bearer <access_token>
    パラメータ
        message=String  最大 1000文字
*******************************************************************************/

#include <Wire.h>                               // I2C通信用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#define SSID "iot-core-esp32"                   // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define BUZZER_PIN    25                        // IO 25にスピーカを接続

/******************************************************************************
 LINE Notify 設定
 ******************************************************************************
 ※LINE アカウントと LINE Notify 用のトークンが必要です。
    1. https://notify-bot.line.me/ へアクセス
    2. 右上のアカウントメニューから「マイページ」を選択
    3. トークン名「esp32」を入力
    4. 送信先のトークルームを選択する(「1:1でLINE Notifyから通知を受け取る」等)
    5. [発行する]ボタンでトークンが発行される
    6. [コピー]ボタンでクリップボードへコピー
    7. 下記のLINE_TOKENに貼り付け
 *****************************************************************************/
#define LINE_TOKEN  "your_token"                // LINE Notify用トークン★要設定

#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角
float Dist = 200;                               // 測定対象までの距離(mm)
float Sobj = 100. * 70. * PI;                   // 測定対象の面積(mm2)
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
int PIR_prev = 0;                               // 人体検知状態の前回の値
int PING_prev = 0;                              // 非接触ボタン状態の前回の値

void send(String S){                            // HTTPS通信でLINEへ送信する
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.begin("https://notify-api.line.me/api/notify");    // アクセス先URL
    http.addHeader("Content-Type","application/x-www-form-urlencoded");
    http.addHeader("Authorization","Bearer " + String(LINE_TOKEN));
    int i = http.POST("message=" + S);          // メッセージをLINEへ送信する
    if(i != 200) M5.Lcd.printf("E(%d) ",i);     // エラー発生時にコードを表示
}

void pir(int in, float dist = 0){
    if(!PIR_prev){                              // 人感検知状態の前回値が0のとき
        String S = "人を検知しました（";        // 文字列変数Sにメッセージ代入
        S += String(dist/10, 0) + "cm）";       // 距離を追記
        send(S);                                // メッセージをLINEで送信
        beep_alert(1);                          // 検知音を鳴らす
    }else{                                      // 入力値inが0のとき(など)
        beep(880, 600);                         // 880(ポン)音を鳴らす
    }
    PIR_prev = in;                              // 今回の値を前回値として更新
}

void pingPong(int in){
    if(in){                                     // 入力値inが1のとき
        send("ボタンが押されました");           // PingをUDP送信
        beep(1109, 600);                        // 1109Hz(ピーン)音を鳴らす
    }else{                                      // 入力値inが0のとき(など)
        beep(880, 600);                         // 880(ポン)音を鳴らす
    }
    PING_prev = in;                             // 今回の値を前回値として更新
}

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    beepSetup(BUZZER_PIN);                      // ブザー用するPWM制御部の初期化
    Wire.begin();                               // I2Cを初期化
    Serial.print("Example 06: Distance Meter [LINE]"); // タイトル表示
    delay(500);                                 // 電源安定待ち時間処理0.5秒
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理
        Serial.print('.');                      // 進捗表示
    }
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
        if(PIR_prev == 1) pir(0);               // 検知中ならPong音を鳴らす
        if(PING_prev == 1) pingPong(0);         // 押下中ならPong音を鳴らす
    }else if(PIR_prev == 0){                    // 人感センサ非検知状態のとき
        pir(1, Dist);                           // PIR=ON(1)と距離をLINEに送信
        beep_alert(1);                          // 検知音を鳴らす
    }else if(Dist <= 100. && PING_prev == 0){   // 100mm以下のとき
        pingPong(1);                            // PingをLINEに送信する
    }else if(Dist >= 150. && PING_prev == 1){   // 150mm以上のとき
        pingPong(0);                            // Pong音を鳴らす
    }

    Serial.printf("Tenv=%.1f ",Tenv);           // 環境温度を表示
    Serial.printf("Tsen=%.1f ",Tsen);           // 測定温度を表示
    Serial.printf("Tobj=%.1f ",Tobj);           // 物体温度を表示
    Serial.printf("Dist=%.0f cm\n",Dist / 10);  // 物体(逆算)距離を表示
}
