/*******************************************************************************
チャイム

int chimeBells(int output, int count);

int output  LEDを接続したIOピン番号
int count   チャイム音が終わるまでのカウントダウン値（0で終了）
int 戻り値  count-1（0以上）

処理時間：約1秒

                                          Copyright (c) 2016-2021 Wataru KUNINO
*******************************************************************************/
#define NOTE_CS6 1109                           // ピン音の周波数
#define NOTE_A5   880                           // ポン音の周波数
#define BEEP_VOL 3                              // スピーカ用の音量(0～10)
#define LED_RED_PIN   16                        // 赤色LEDのIOポート番号
#define LED_GREEN_PIN 17                        // 緑色LEDのIOポート番号
int BEEP_PIN = -1; // 初期化有無

/* ESP8266用
int beep_chimeBells(int output, int count) {
    int t;
    if(count==0)return 0;
    if(!(count%2)){
        tone(output,NOTE_CS6,800);
        for(t=0;t<8;t++) delay(100);
        noTone(output);
        for(t=0;t<2;t++) delay(100);
    }else{
        tone(output,NOTE_A5,800);
        for(t=0;t<8;t++) delay(100);
        noTone(output);
        for(t=0;t<2;t++) delay(100);
    }
    count--;
    if(count<0) count=0;
    return(count);
}
*/

#ifdef _M5STACK_H_
void beep(int freq, int t){
    if(BEEP_PIN < 0){
        pinMode(LED_RED_PIN, OUTPUT);           // GPIO 16 を赤色LED用に設定
        pinMode(LED_GREEN_PIN, OUTPUT);         // GPIO 17 を緑色LED用に設定
        digitalWrite(LED_RED_PIN, LOW);         // LED赤を消灯
        digitalWrite(LED_GREEN_PIN, LOW);       // LED緑を消灯
        BEEP_PIN = 25;
    }
    if(!BEEP_VOL){
        M5.Lcd.invertDisplay(false);            // 画面を反転する
        delay(t);
        M5.Lcd.invertDisplay(true);             // 画面の反転を戻す
        return;
    }
    M5.Speaker.begin();                         // M5Stack用スピーカの起動
    for(int vol = BEEP_VOL; vol > 0; vol--){    // 繰り返し処理(6回)
        M5.Speaker.setVolume(vol);              // スピーカの音量を設定
        M5.Speaker.tone(freq);                  // スピーカ出力freq Hzの音を出力
        delay(t / BEEP_VOL);                    // 0.01秒(10ms)の待ち時間処理
    }
    M5.Speaker.end();                           // スピーカ出力を停止する
}
#else // M5
/* スピーカ出力用 LEDC */
#define LEDC_CHANNEL_0     0    // use first channel of 16 channels (started from zero)
#define LEDC_TIMER_13_BIT  13   // use 13 bit precission for LEDC timer
#define LEDC_BASE_FREQ     5000 // use 5000 Hz as a LEDC base frequency

void beepSetup(int PIN){
    BEEP_PIN = PIN;
    pinMode(BEEP_PIN,OUTPUT);                   // スピーカのポートを出力に
    pinMode(LED_RED_PIN, OUTPUT);               // GPIO 16 を赤色LED用に設定
    pinMode(LED_GREEN_PIN, OUTPUT);             // GPIO 17 を緑色LED用に設定
    digitalWrite(LED_RED_PIN, LOW);             // LED赤を消灯
    digitalWrite(LED_GREEN_PIN, LOW);           // LED緑を消灯
    Serial.print("ledSetup LEDC_CHANNEL_0 = ");
    Serial.print(LEDC_CHANNEL_0);
    Serial.print(", BEEP_PIN = ");
    Serial.print(BEEP_PIN);
    Serial.print(", freq. = ");
    Serial.println(ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT),3);
    ledcAttachPin(PIN, LEDC_CHANNEL_0);
}
void beep(int freq, int t){                     // ビープ音を鳴らす関数
    if(BEEP_PIN < 0) beepSetup(25);            // ポート25で初期化
    ledcWriteTone(0, freq);                     // PWM出力を使って音を鳴らす
    for(int duty = 50; duty > 1; duty /= 2){    // PWM出力のDutyを減衰させる
        ledcWrite(0, BEEP_VOL * duty / 10);     // 音量を変更する
        delay(t / 6);                           // 0.1秒(100ms)の待ち時間処理
    }
    ledcWrite(0, 0);                            // ビープ鳴音の停止
}
#endif //M5

void beep(int freq){
    beep(freq, 100);
    digitalWrite(LED_RED_PIN, LOW);             // LED赤を消灯
    digitalWrite(LED_GREEN_PIN, LOW);           // LED緑を消灯
}

void beep_chime(){
    digitalWrite(LED_RED_PIN, LOW);             // LED赤を消灯
    digitalWrite(LED_GREEN_PIN, HIGH);          // LED緑を点灯
    int t;
    beep(NOTE_CS6, 600);
    for(t=0;t<3;t++) delay(100);
    beep(NOTE_A5, 100);
}

void beep_alert(int num){
    digitalWrite(LED_RED_PIN, HIGH);            // LED赤を点灯
    digitalWrite(LED_GREEN_PIN, LOW);           // LED緑を消灯
    for(; num > 0 ; num--) for(int i = 2217; i > 200; i /= 2) beep(i, 100);
}

void beep_alert(){
    beep_alert(3);
}
