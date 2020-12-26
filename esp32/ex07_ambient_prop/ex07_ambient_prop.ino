/*******************************************************************************
Example 07: NCIR MLX90614 & TOF Human Body Temperature Checker

・距離センサが人体を検出すると、測定を開始します。
・37.5℃以上だった場合、警告音を鳴らします。
・35.0～37.5℃だった場合、ピンポン音を鳴らします。
・測定値をAmbientへ送信します。

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

float TempWeight = 1110.73;                     // 温度(利得)補正係数
float TempOffset = 36.5;                        // 温度(加算)補正係数
float DistOffset = 29.4771;                     // 距離補正係数
int PIR_prev = 0;                               // 人体検知状態
float temp_sum = 0.0;                           // 体温値の合計(平均計算用)
int temp_count = 0;                             // temp_sumの測定済サンプル数

void send(int pir, float temp){                 // HTTP通信でAmbientへ送信する
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
        + String(temp)                           // 　　(値)温度値
        + "\",\"d2\":\""                        // (項目名)d2
        + String(pir)                           // 　　(値)PIRセンサ値
        + "\"}"
    );
    if(i == 200) Serial.print(temp, 1);         // 送信した温度値を表示
    else Serial.printf("E(%d) ",i);             // エラー表示
    PIR_prev = pir;                             // 人体検知状態を更新
}

void setup(){                                   // 起動時に一度だけ実行する関数
    Serial.begin(115200);                       // シリアル通信速度を設定する
    beepSetup(BUZZER_PIN);                      // ブザー用するPWM制御部の初期化
    Wire.begin();                               // I2Cを初期化
    Serial.println("Example 07: Body Temperature Checker [ToF][Ambient]");
    delay(500);                                 // 電源安定待ち時間処理0.5秒
    WiFi.mode(WIFI_STA);                        // 無線LANを【子機】モードに設定
    WiFi.begin(SSID,PASS);                      // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){       // 接続に成功するまで待つ
        delay(500);                             // 待ち時間処理
        Serial.print('.');                      // 進捗表示
    }
}

void loop(){                                    // 繰り返し実行する関数
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20.) return;                     // 20mm以下の時に再測定
    if(Dist > 400){                             // 400mm超のとき
        if(PIR_prev == 1 && temp_count > 0){    // 人体検知中で測定値がある時
            send(0,temp_sum/(float)temp_count); // Ambientへ送信
        }
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
        Serial.printf("Tavr=%.1f ",temp_avr);   // 平均温度を表示
        PIR_prev = 1;                           // 人体検知状態を検知に
        beep(1047);                             // 1047Hzのビープ音(測定中)
    }
    if(temp_count % 20 != 0) return;            // 剰余が0以外のときに先頭へ
    
    if(temp_avr >= 37.5){                       // 37.5℃以上のとき(発熱検知)
        send(1,temp_avr);                       // 発熱値をAmbientへ送信
        beep_alert(3);                          // アラート音を3回、鳴らす
    }else if(temp_avr < 35.0){                  // 35.0℃未満のとき(再測定)
        temp_sum = Tobj;                        // 最後の測定結果のみを代入
        temp_count = 1;                         // 測定済サンプル数を1に
    }else{
        send(0,temp_avr);                       // 体温の平均値をAmbientへ送信
        beep_chime();                           // ピンポン音を鳴らす
        temp_sum = 0.0;                         // 体温の合計値を0にリセット
        temp_count = 0;                         // 測定サンプル数を0にリセット
        delay(3000);                            // 3秒間、待機する
    }
}
