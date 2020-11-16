/*******************************************************************************
Example 03: NCIR MLX90614 & TOF Object Temperature Meter for M5Stack
・非接触温度センサ の読み値をアナログ・メータ表示します
・測距センサを使って測定対象までの距離を求め、温度値を補正します。
・Melexis; Microelectronic Integrated Systems, Infra Red Thermometer

                                          Copyright (c) 2019-2020 Wataru KUNINO
********************************************************************************
【参考文献】

Arduino IDE 開発環境イントール方法：
    https://github.com/m5stack/M5Stack/blob/master/docs/getting_started_ja.md
    https://docs.m5stack.com/#/en/related_documents/Arduino_IDE

M5Stack Arduino Library API 情報：
    https://docs.m5stack.com/#/ja/api
    https://docs.m5stack.com/#/en/arduino/arduino_api

MLX90614
    MLX90614xAA h=4.1mm 90°
    MLX90614xBA h=4.1mm 70°Dualタイプ
    MLX90614xCC h=8.1mm 35°
    MLX90614xCF h=17.2mm 10°

TOF STMicroelectronics VL53L0X
    https://groups.google.com/d/msg/diyrovers/lc7NUZYuJOg/ICPrYNJGBgAJ
*******************************************************************************/

#include <M5Stack.h>                            // M5Stack用ライブラリ
#include <Wire.h>                               // I2C通信用ライブラリ
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角×補正係数
float Dist = 200;                               // 測定対象までの距離(mm)
float Area = 100. * 70. * PI;                   // 測定対象の面積(mm2)
char csvfile[10] = "/ncir.csv";
char bmpfile[10] = "/ncir.bmp";

void printTitle(){
    M5.Lcd.fillRect(0, 21 * 8, 320, 8, 0);      // 描画位置の文字を消去(0=黒)
    M5.Lcd.setCursor(0, 21 * 8);                // 液晶描画位置をlcd_row行目に
    M5.Lcd.print("Example 03: Object Temperature Meter [ToF][Graph]");
}

float getTemp(byte reg = 0x7){
    int16_t val = 0xFFFF;                       // 変数valを定義
    Wire.beginTransmission(0x5A);               // MLX90614(0x5A)との通信を開始
    Wire.write(reg);                            // レジスタ番号を指定
    if(Wire.endTransmission(false)==0){         // MLX90614(0x5A)との通信を継続
        Wire.requestFrom(0x5A, 2);              // 2バイトのデータを要求
        if(Wire.available() >= 2){              // 2バイト以上を受信
            val = (int16_t)Wire.read();         // 1バイト目を変数tempの下位へ
            val |= ((int16_t)Wire.read()) << 8; // 2バイト目を変数tempの上位へ
        }
    }
    return (float)val * 0.02 - 273.15;          // 温度に変換して応答
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC", "NCIR", 0, 40);     // メータのレンジおよび表示設定
    printTitle();
    File file = SD.open(csvfile, "w");
    if(file){
        file.print("Dist, Tenv, Tsen, Tobj\n");
        file.close();
    }
}

void beep(int freq){
    M5.Speaker.begin();         // M5Stack用スピーカの起動
    M5.Speaker.tone(freq);      // スピーカ出力 freq Hzの音を出力
    delay(10);
    M5.Speaker.end();           // スピーカ出力を停止する
}

int temp2yaxis(float temp, float min = 20., float max = 40.){
	// 温度値を y軸の値（232～0）に
    int y;
    if(temp < min) return 232;
    if(temp > max) return 0;
    return 232 - (int)(232. * (temp - min) / (max - min) + .5);
}

int lcd_row = 22;                               // 液晶画面上の行数保持用の変数
int mode = 1;                                   // モード 1:温度 2:距離 3:グラフ
void loop(){                                    // 繰り返し実行する関数
    /* 操作部 */
    delay(100);                                 // 0.1秒（100ms）の待ち時間処理
    M5.update();                                // ボタン情報を更新
    if(M5.BtnA.wasPressed() && mode != 1){      // ボタンAでモード1(温度)
        mode = 1; beep(880);                    // モード2(温度測定)に設定
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        analogMeterInit("degC", "NCIR", 0, 40); // メータのレンジおよび表示設定
        printTitle();
    }
    if(M5.BtnB.wasPressed() && mode != 2){      // ボタンBが押された時
        mode = 2; beep(880);                    // モード2(距離測定)に設定
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        analogMeterInit("cm", "ToF", 0, 400);   // メータのレンジおよび表示設定
        printTitle();
    }
    if(mode == 3 && M5.BtnC.read()){
        delay(1000);
        if(M5.BtnC.read()){
            beep(880);                  // スピーカ出力 880Hzの音を出力
            bmpScreenServer(bmpfile);   // スクリーンショットを保存
            beep(1047);                 // スピーカ出力 1047Hzの音を出力
        }
    }
    if(M5.BtnC.wasPressed() && mode != 3){      // ボタンCが押された時
        mode = 3; beep(880);                    // モード2(距離測定)に設定
        M5.Lcd.fillScreen(BLACK);               // LCDを消去
        M5.Lcd.drawRect(0, 0, 320, 232, BLUE);  // 座標0,0から320x234の箱を描画
        for(int y = 0; y < 231; y += 29) M5.Lcd.drawLine(0,y,319,y, BLUE);
        for(int x = 0; x < 319; x += 40) M5.Lcd.drawLine(x,0,x,231, BLUE);
        M5.Lcd.drawLine(0,203,319,203, RED);
    }
    /* 測定部 */
    Dist = (float)VL53L0X_get();                // 測距センサVL53L0Xから距離取得
    float Tenv, Tsen, Tobj, Ssen;
    if(Dist <= 20. || Dist > 8000) return;      // 20mm以下/8000mm超のときに戻る
    if(mode != 2){                              // 距離測定モード以外のとき
        Tenv= getTemp(6);                       // センサの環境温度を取得
        if(Tenv < 0) return;                    // 0℃未満のときは先頭に戻る
        Tsen= getTemp();                        // センサの測定温度を取得
        if(Tsen < 0) return;                    // 0℃未満のときは先頭に戻る
        Ssen= pow(Dist * tan(FOV / 360. * PI), 2.) * PI;      // 測定点の面積
        Tobj = Tsen;                                          // 温度測定結果
        if(Area < Ssen) Tobj = (Tsen - Tenv) * Ssen / Area + Tenv;  // 面積比で補正
        if(Tobj < 0. || Tobj > 99.) return;     // 0℃未満/99℃超過時は戻る
    }
    /* 結果表示 */
    switch(mode){
      case 1:   /* 非接触温度測定表示 */
        M5.Lcd.setCursor(0,lcd_row * 8);            // 液晶描画位置をlcd_row行目に
        M5.Lcd.printf("%.0fcm, ",Dist/10);                    // 測距結果を表示
        Serial.printf("%.1fcm, ",Dist/10);                    // 測距結果を出力
        M5.Lcd.printf("Te=%.1f, ",Tenv);                      // 環境温度を表示
        Serial.printf("Te=%.2f, ",Tenv);                      // 環境温度を出力
        M5.Lcd.printf("Ts=%.1f(%.0fcm2), ",Tsen, Ssen / 100); // 測定温度を表示
        Serial.printf("Ts=%.2f(%.0fcm2), ",Tsen, Ssen / 100); // 測定温度を出力
        M5.Lcd.printf("To=%.1f(%.0fcm2)"  ,Tobj, Area / 100); // 物体温度を表示
        Serial.printf("To=%.2f(%.0fcm2)\n",Tobj, Area / 100); // 物体温度を出力
        analogMeterNeedle(Tobj);                    // 温度値をメータ表示
        break;
      case 2:     /* 距離メータ風表示 */
        M5.Lcd.setCursor(0,lcd_row * 8);            // 液晶描画位置をlcd_row行目に
        M5.Lcd.printf("%.1fcm  ",Dist/10);              // 測距結果を表示
        Serial.printf("%.1fcm\n",Dist/10);              // 測距結果を出力
        analogMeterNeedle(Dist/10);                     // 距離をメータ表示
        break;
      case 3:     /* 温度vs距離グラフ表示 */
        int x = (int)(Dist * 320. / 400.);              // 最大値=400mm
        M5.Lcd.drawPixel(x, temp2yaxis(Tsen-Tenv,-2,14), GREEN);     // 測定温度
        int color = WHITE;
        if(Tsen - Tenv < 1.0) color =RED;
        M5.Lcd.drawPixel(x, temp2yaxis(Tobj-Tenv,-2,14), color);     // 物体温度
        
        File file = SD.open(csvfile, "a");
        if(file){
            char s[256];
            snprintf(s, 256, "%.1f, %.2f, %.2f, %.2f\n", Dist, Tenv, Tsen, Tobj);
            file.print(s);
            file.close();
        }
        break;
    }
    lcd_row++;                                  // 行数に1を加算する
    if(lcd_row > 29) lcd_row = 22;              // 最下行まで来たら先頭行へ
    if(mode != 3) M5.Lcd.fillRect(0, lcd_row * 8, 320, 8, 0); // 描画位置の文字を消去(0=黒)
}
