/*******************************************************************************
Example 03: NCIR MLX90614 & TOF Human Body Temperature Meter for M5Stack
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
#define BEEP 0                                  // ビープ音
#define ChartYmax 12                            // グラフ最大値
#define ChartYmin -4                            // グラフ最小値
float TempWeight = 1110.73;                     // 温度(利得)補正係数
float TempOffset = 36.0;                        // 温度(加算)補正係数
float DistOffset = 29.4771;                     // 距離補正係数
char csvfile[10] = "/ncir.csv";
char bmpfile[10] = "/ncir.bmp";
int lcd_row = 22;                               // 液晶画面上の行数保持用の変数
int mode = 1;                                   // 1:体温 2:NICR 3:距離 4:グラフ

void printMenu(){
    M5.Lcd.fillRect(0, 224, 320, 16, BLUE);
    M5.Lcd.setCursor(40, 224, 2);
    M5.Lcd.printf("Mode(%d)",mode);
    M5.Lcd.setCursor(132, 224, 2);
    M5.Lcd.printf("    -");
    M5.Lcd.setCursor(226, 224, 2);
    M5.Lcd.printf("    +");
    M5.Lcd.setCursor(188, 224, 2);
    M5.Lcd.printf("[%2.1f]",TempOffset);
    M5.Lcd.setCursor(0, 22 * 8, 1);             // 液晶描画位置をlcd_row行目に
}

void printTitle(){
    M5.Lcd.fillRect(0, 21 * 8, 320, 8, 0);      // 描画位置の文字を消去(0=黒)
    M5.Lcd.setCursor(0, 21 * 8, 1);             // 液晶描画位置をlcd_row行目に
    M5.Lcd.print("Example 03: Object Temperature Meter [ToF][Graph]");
    printMenu();
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
    analogMeterInit("degC", "Face", 20, 40);    // メータのレンジおよび表示設定
    printTitle();
    File file = SD.open(csvfile, "w");
    if(file){
        file.print("Dist, Tenv, Tsen, Tobj\n");
        file.close();
    }
    M5.Lcd.invertDisplay(true);
}

void beep(int freq){
    if(BEEP > 0){
        M5.Speaker.begin();                     // M5Stack用スピーカの起動
        M5.Speaker.tone(freq);                  // スピーカ出力 freq Hzの音を出力
        delay(10);
        M5.Speaker.end();                       // スピーカ出力を停止する
    }else{
        M5.Lcd.invertDisplay(false);
        delay(100);
        M5.Lcd.invertDisplay(true);
    }
}

int temp2yaxis(float temp, float min = 20., float max = 40.){
    // 温度値を y軸の値（224～0）に
    int y;
    if(temp < min) return 224;
    if(temp > max) return 0;
    return 224 - (int)(224. * (temp - min) / (max - min) + .5);
}

void printMode(int mode){
    if(mode < 1 || mode > 4) mode = 1;
    char mode_s[4][5]={"Face","NCIR","TOF ","Test"};
    M5.Lcd.fillRect(0, 224, 320, 16, BLUE);
    M5.Lcd.setCursor(40, 224, 2);
    M5.Lcd.printf("Mode(%d) = %s",mode,mode_s[mode - 1]);
}

void loop(){                                    // 繰り返し実行する関数
    /* 操作部 */
    delay(100);                                 // 0.1秒（100ms）の待ち時間処理
    M5.update();                                // ボタン情報を更新
    if(M5.BtnB.isPressed()){
        TempOffset -= 0.1;
        if(TempOffset < 30.) TempOffset = 30.;
        printMenu();
    }
    if(M5.BtnC.isPressed()){
        TempOffset += 0.1;
        if(TempOffset > 40.) TempOffset = 40.;
        printMenu();
    }
    if(M5.BtnA.wasPressed()){                   // ボタンAでモード切替
        beep(880);
        unsigned long t = millis();
        while(M5.BtnA.read()) delay(100);
        if(millis() - t > 3000){
            bmpScreenServer(bmpfile);           // スクリーンショットを保存
            beep(1047);
            return;
        }
        mode += 1;
        if(mode < 1 || mode > 4) mode = 1;
        printMode(mode);
        for(int i = 0; i < 20; i++){
            if(M5.BtnA.read()){
                i = 0;
                mode += 1;
                if(mode < 1 || mode > 4) mode = 1;
                printMode(mode);
                while(M5.BtnA.read()) delay(100);
            }
            delay(100);
        }
        M5.Lcd.setCursor(0, 0, 1);
        beep(1047);
        if(mode == 1){                              // モード1(温度)
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("degC", "Face", 30, 40);   // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == 2){
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("degC", "NCIR", 0, 40); // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == 3){
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("cm", "ToF", 0, 400);   // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == 4){
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            M5.Lcd.drawRect(0, 0, 320, 225, NAVY);  // 座標0,0から320x224の箱を描画
            for(int y = 0; y < 224; y += 28) M5.Lcd.drawLine(0,y,319,y, NAVY);
            for(int x = 0; x < 319; x += 40) M5.Lcd.drawLine(x,0,x,223, NAVY);
            M5.Lcd.drawLine(0,196,319,196, RED);
            printMenu();
        }
    }
        
    /* 測定部 */
    float Tenv = nan(NULL), Tsen = nan(NULL), Tobj = nan(NULL);
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20. || Dist > 8000) return;      // 20mm以下/8000mm超のときに戻る
    if(mode != 3){                              // 距離測定モード以外のとき
        Tenv= getTemp(6);                       // センサの環境温度を取得
        if(Tenv < 0) return;                    // 0℃未満のときは先頭に戻る
        Tsen= getTemp();                        // センサの測定温度を取得
        if(Tsen < 0) return;                    // 0℃未満のときは先頭に戻る
        // 体温計算 温度利得 ÷ 距離 ＋ 温度差
        Tobj = Tsen - Tenv + TempOffset;
        Tobj -= TempWeight / (Dist + DistOffset); // 距離補正
        if(Tobj < 0. || Tobj > 99.) return;     // 0℃未満/99℃超過時は戻る
    }

    /* 結果表示 */
    Serial.printf("%.1fcm, ",Dist/10);          // 測距結果を出力
    Serial.printf("Te=%.2f, ",Tenv);            // 環境温度を出力
    Serial.printf("Ts=%.2f, ",Tsen);            // 測定温度を出力
    Serial.printf("To=%.2f\n",Tobj);            // 物体温度を出力
    switch(mode){
        case 1:
            analogMeterNeedle(Tobj);            // 体温値をメータ表示
            break;
        case 2:
            analogMeterNeedle(Tsen);            // NICR温度値をメータ表示
            break;
        case 3:     /* 距離メータ表示 */
            analogMeterNeedle(Dist/10);                     // 距離をメータ表示
            break;
        case 4:     /* グラフ表示 */
            int x = (int)(Dist * 320. / 400.);          // 最大値=400mm
            if(x < 0) x = 0;
            if(x > 319) x = 319;
            M5.Lcd.drawPixel(x, temp2yaxis(Tobj - TempOffset,ChartYmin,ChartYmax), GREEN);
            M5.Lcd.drawPixel(x, temp2yaxis(Tsen - Tenv,ChartYmin,ChartYmax), WHITE);
            
            File file = SD.open(csvfile, "a");
            if(file){
                char s[256];
                snprintf(s, 256, "%.1f, %.2f, %.2f, %.2f\n", Dist, Tenv, Tsen, Tobj);
                file.print(s);
                file.close();
                Serial.print(s);
            }
            return;
    }
    M5.Lcd.setCursor(0,lcd_row * 8);            // 液晶描画位置をlcd_row行目に
    if(mode != 2) M5.Lcd.printf("%.0fcm, ",Dist/10);          // 測距結果を表示
    if(mode != 3) M5.Lcd.printf("Te=%.1f, ",Tenv);            // 環境温度を表示
    if(mode != 3) M5.Lcd.printf("Ts=%.1f, ",Tsen);            // 測定温度を表示
    if(mode == 1) M5.Lcd.printf("To=%.1f"  ,Tobj);            // 物体温度を表示
    lcd_row++;                                  // 行数に1を加算する
    if(lcd_row > 27) lcd_row = 22;              // 最下行まで来たら先頭行へ
    M5.Lcd.fillRect(0, lcd_row * 8, 320, 8, 0); // 描画位置の文字を消去(0=黒)
}
