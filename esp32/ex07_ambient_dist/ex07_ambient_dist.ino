/*******************************************************************************
Example 06: NCIR MLX90614 Human Face Distance Meter

・非接触温度センサ の読み値から顔までの距離を求めます(測距センサ不要)。
・距離が40cm以下になった時に、検出音を鳴らします。
・手のひらなどをセンサに近づけた時に、ピンポン音を鳴らします。
　（屋外などで掌が冷えているときは検知できません）
・測定値をAmbientへ送信します(約30秒に1度の頻度)。

・対応する非接触温度センサ：
　M5Stack NCIR Non-Contact Infrared Thermometer Sensor Unit
　Melexis MLX90614; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2020-2021 Wataru KUNINO
********************************************************************************
【参考文献】

NCIRセンサ MLX90614 (Melexis製)
    https://www.melexis.com/en/product/MLX90614/
    MLX90614xAA (5V仕様：x=A, 3V仕様：x=B) h=4.1mm 90°

IoTデータ可視化サービスAmbient(アンビエントデーター社)
    https://ambidata.io/
*******************************************************************************/

#include <Wire.h>                               // I2C通信用ライブラリ
#include <WiFi.h>                               // ESP32用WiFiライブラリ
#include <HTTPClient.h>                         // HTTPクライアント用ライブラリ
#define SSID "iot-core-esp32"                   // 無線LANアクセスポイントのSSID
#define PASS "password"                         // パスワード
#define Amb_Id  "00000"                         // AmbientのチャネルID 
#define Amb_Key "0000000000000000"              // Ambientのライトキー
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
unsigned long Counter = 0;                      // 送信用カウンタ

void send(int pir, float dist){                 // HTTP通信でAmbientへ送信する
    HTTPClient http;                            // HTTPリクエスト用インスタンス
    http.begin(                                 // アクセス先のURLを設定
        "http://ambidata.io/api/v2/channels/"   // URL
        + String(Amb_Id)                        // チャネルID
        + "/data"                               // データ送信
    );
    http.addHeader(                             // HTTPヘッダを設定
        "Content-Type",                         // (項目名)データ形式
        "application/json"                      // 　　(値)JSON
    );
    int i = http.POST(                          // HTTP POSTを実行
        "{\"writeKey\":\""                      // (項目名)writeKey
        + String(Amb_Key)                       // 　　(値)Ambientのライトキー
        + "\",\"d1\":\""                        // (項目名)d1
        + String(dist / 10)                     // 　　(値)距離
        + "\",\"d2\":\""                        // (項目名)d2
        + String(pir)                           // 　　(値)PIRセンサ値
        + "\"}"
    );
    if(i == 200) Serial.print(dist / 10, 1);    // 送信した温度値を表示
    else Serial.printf("E(%d) ",i);             // エラー表示
}

void pir(int in, float dist = 0){
    if(!PIR_prev){                              // 人感検知状態の前回値が0のとき
        send(1, dist);                          // Ambientへ送信
        beep_alert(1);                          // 検知音を鳴らす
    }else{                                      // 入力値inが0のとき(など)
        beep(880, 600);                         // 880(ポン)音を鳴らす
    }
    PIR_prev = in;                              // 今回の値を前回値として更新
}

void pingPong(int in){
    if(in){                                     // 入力値inが1のとき
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
    Serial.print("Example 06: Distance Meter [Ambient]"); // タイトル表示
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
    if(millis() - Counter > 30000){             // 30秒(30000ms)以上経過
        Counter = millis();                     // 現在時刻を変数Counterに保持
        send(PIR_prev, Dist);                   // Ambientへ送信
    }
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

    Serial.setCursor(0,LCD_row * 8);            // 液晶描画位置をLCD_row行目に
    Serial.printf("Tenv=%.1f ",Tenv);           // 環境温度を表示
    Serial.printf("Tsen=%.1f ",Tsen);           // 測定温度を表示
    Serial.printf("Tobj=%.1f ",Tobj);           // 物体温度を表示
    Serial.printf("Dist=%.0f cm ",Dist / 10);   // 物体(逆算)距離を表示
}
