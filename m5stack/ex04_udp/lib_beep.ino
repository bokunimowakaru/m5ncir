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

/*
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

void beep(int freq, int t){
    M5.Lcd.invertDisplay(false);                // 画面を反転
    M5.Speaker.begin();                         // M5Stack用スピーカの起動
    M5.Speaker.tone(freq);                      // スピーカ出力 freq Hzの音を出力
    delay(t);                                   // 0.1秒(100ms)の待ち時間処理
    M5.Speaker.end();                           // スピーカ出力を停止する
    M5.Lcd.invertDisplay(true);                 // 画面の反転を戻す
}

void beep(int freq){
    beep(freq, 100);
}

void beep_chime(){
    int t;
    beep(NOTE_CS6, 600);
    for(t=0;t<3;t++) delay(100);
    beep(NOTE_A5, 100);
}

void beep_alert(){
    for(int j = 0; j < 3 ; j++) for(int i = 2217; i > 200; i /= 2) beep(i);
}
