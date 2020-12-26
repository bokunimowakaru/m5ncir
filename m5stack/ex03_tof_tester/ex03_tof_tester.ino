/*******************************************************************************
Example 03: NCIR MLX90614 & TOF Human Body Temperature Meter for M5Stack

・非接触温度センサ の読み値を体温に変換し、アナログ・メータ表示します。
・測距センサを使って顔までの距離を測定し、1次変換式により体温を算出します。
・左ボタンで下記のモード切替、長押しでスクリーンショットをマイクロSDカードへ保存
        1:体温(面積換算) 
        2:体温(係数換算) 
        3:NICRセンサ値 
        4:TOFセンサ値 
        5:グラフ表示（センサ値データはマイクロSDカードへ書き込む）
        長押し＝スクリーンショットをマイクロSDカードへ保存

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
#ifndef PI
    #define PI 3.1415927                        // 円周率
#endif
#define FOV 90.                                 // センサの半値角
#define ChartYmax 12                            // グラフ最大値
#define ChartYmin -4                            // グラフ最小値
float TempWeight = 1110.73;                     // 温度(利得)補正係数
float TempOffset = 36.5;                        // 温度(加算)補正係数
float TempOfsAra = (273.15 + 36) * 0.02;        // 皮膚からの熱放射時の減衰
float DistOffset = 29.4771;                     // 距離補正係数
char csvfile[10] = "/ncir.csv";
char bmpfile[10] = "/ncir.bmp";
int LCD_row = 22;                               // 液晶画面上の行数保持用の変数
enum Mode {Area=1, Prop=2, NCIR=3, TOF=4, Chrt=5, None=0} mode = Area;
// モード 1:体温(面積換算) 2:体温(係数換算) 3:NICRセンサ値 4:TOFセンサ値 5:グラフ

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
    M5.Lcd.setCursor(0, 22 * 8, 1);             // 液晶描画位置をLCD_row行目に
}

void printTitle(){
    M5.Lcd.fillRect(0, 21 * 8, 320, 8, 0);      // 描画位置の文字を消去(0=黒)
    M5.Lcd.setCursor(0, 21 * 8, 1);             // 液晶描画位置をLCD_row行目に
    M5.Lcd.print("Example 03: Body Temperature Meter [ToF][Graph]");
    printMenu();
}

void setup(){                                   // 起動時に一度だけ実行する関数
    M5.begin();                                 // M5Stack用ライブラリの起動
    Wire.begin();                               // I2Cを初期化
    M5.Lcd.setBrightness(100);                  // LCDの輝度を100に設定
    analogMeterInit("degC", "Face Area", 30, 40);    // メータのレンジおよび表示設定
    printTitle();
    File file = SD.open(csvfile, "w");
    if(file){
        file.print("Dist, Tenv, Tsen, TobjAra, TobjPrp\n");
        file.close();
    }
    M5.Lcd.invertDisplay(true);
}

int temp2yaxis(float temp, float min = 20., float max = 40.){
    // 温度値を y軸の値（224～0）に
    int y;
    if(temp < min) return 224;
    if(temp > max) return 0;
    return 224 - (int)(224. * (temp - min) / (max - min) + .5);
}

void printMode(int mode){
    if(mode < 1 || mode > 5) mode = 1;
    char mode_s[5][5]={"Area","Prop","NCIR","TOF ","Chrt"};
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
        TempOfsAra = TempOffset - (36. - 5.);
        printMenu();
    }
    if(M5.BtnC.isPressed()){
        TempOffset += 0.1;
        if(TempOffset > 40.) TempOffset = 40.;
        TempOfsAra = TempOffset - (36. - 5.);
        printMenu();
    }
    if(M5.BtnA.wasPressed()){                   // ボタンAでモード切替
        beep(880);
        unsigned long t = millis();
        while(M5.BtnA.read()){
            delay(100);
            if(millis() - t > 3000){            // 3秒間の長押しを検出
                beep(1047);
                bmpScreenServer(bmpfile);       // スクリーンショットを保存
                beep_chime();
                return;
            }
        }
        mode = (Mode)((int)mode + 1);
        if((int)mode < 1 || (int)mode > 5) mode = (Mode)1;
        printMode((int)mode);
        for(int i = 0; i < 20; i++){
            if(M5.BtnA.read()){
                i = 0;
                mode = (Mode)((int)mode + 1);
                if((int)mode < 1 || (int)mode > 5) mode = (Mode)1;
                printMode((int)mode);
                while(M5.BtnA.read()) delay(100);
            }
            delay(100);
        }
        M5.Lcd.setCursor(0, 0, 1);
        beep(1047);
        if(mode == Area){                           // モード 1:体温(面積換算) 
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("degC", "Face Area", 30, 40);   // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == Prop){                           // モード 2:体温(係数換算)
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("degC", "Face Prop", 30, 40);   // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == NCIR){                           // モード 3:NICRセンサ値
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("degC", "NCIR", 0, 40); // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == TOF){                            // モード 4:TOFセンサ値
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            analogMeterInit("cm", "ToF", 0, 80);    // メータのレンジおよび表示設定
            printTitle();
        }
        if(mode == Chrt){                           // モード 5:グラフ
            M5.Lcd.fillScreen(BLACK);               // LCDを消去
            M5.Lcd.drawRect(0, 0, 320, 225, NAVY);  // 座標0,0から320x224の箱を描画
            for(int y = 0; y < 224; y += 28) M5.Lcd.drawLine(0,y,319,y, NAVY);
            for(int x = 0; x < 319; x += 40) M5.Lcd.drawLine(x,0,x,223, NAVY);
            M5.Lcd.drawLine(0,168,319,168, RED);
            printMenu();
        }
    }
        
    /* 測定部 */
    float Tenv = nan(NULL), Tsen = nan(NULL);   // センサ値代入用
    float TobjAra = nan(NULL);                  // 体温値TobjAra(面積換算方式)
    float TobjPrp = nan(NULL);                  // 体温値TobjPrp(係数換算方式)
    float Ssen, Sobj;                           // 面積の保持用
    float Dist = (float)VL53L0X_get();          // 測距センサVL53L0Xから距離取得
    if(Dist <= 20. || Dist > 8000) return;      // 20mm以下/8000mm超のときに戻る
    if(mode != TOF){                            // 距離測定モード以外のとき
        Tenv= getTemp(6);                       // センサの環境温度を取得
        Tsen= getTemp();                        // センサの測定温度を取得
        if(Tenv < -20. || Tsen < -20.) return;  // -20℃未満のときは中断
        
        // 面積換算方式
        // 体温TobjAra = 環境温度 + センサ温度差値×√(センサ測定面積÷測定対象面積)
        Ssen = pow(Dist * tan(FOV / 360. * PI), 2.) * PI;  // センサ測定面積
        Sobj = 100. * 70. * PI;           // 測定対象面積
        TobjAra = Tenv + TempOfsAra + (Tsen - Tenv) * sqrt(Ssen / Sobj);
        
        // 係数換算方式
        // 体温TobjPrp = 基準温度 + センサ温度差値 - 温度利得 ÷ 距離
        TobjPrp = TempOffset + (Tsen - Tenv) - (TempWeight / (Dist + DistOffset));
    }

    /* 結果表示 */
    Serial.printf("%.1fcm, ",Dist/10);          // 測距結果を出力
    Serial.printf("Te=%.2f, ",Tenv);            // 環境温度を出力
    Serial.printf("Ts=%.2f, ",Tsen);            // 測定温度を出力
    Serial.printf("ToAra=%.2f, ",TobjAra);      // 物体温度を出力
    Serial.printf("ToPrp=%.2f, ",TobjPrp);      // 物体温度を出力
    Serial.println();
    switch(mode){
        case Area:
            analogMeterNeedle(TobjAra);         // 体温値をメータ表示
            break;
        case Prop:
            analogMeterNeedle(TobjPrp);         // 体温値をメータ表示
            break;
        case NCIR:
            analogMeterNeedle(Tsen);            // NICR温度値をメータ表示
            break;
        case TOF:     /* 距離メータ表示 */
            analogMeterNeedle(Dist/10);                     // 距離をメータ表示
            break;
        case Chrt:     /* グラフ表示 */
            int x = (int)(Dist * 320. / 400.);          // 最大値=400mm
            if(x < 0) x = 0;
            if(x > 319) x = 319;
            M5.Lcd.drawPixel(x, temp2yaxis(TobjAra-TempOffset,ChartYmin,ChartYmax), RED);
            M5.Lcd.drawPixel(x, temp2yaxis(TobjPrp-TempOffset,ChartYmin,ChartYmax), GREEN);
            M5.Lcd.drawPixel(x, temp2yaxis(Tsen-Tenv,ChartYmin,ChartYmax), WHITE);
            File file = SD.open(csvfile, "a");
            if(file){
                char s[256];
                snprintf(s, 256, "%.1f, %.2f, %.2f, %.2f, %.2f\n", Dist, Tenv, Tsen, TobjAra, TobjPrp);
                file.print(s);
                file.close();
                Serial.print(s);
            }
            return;
    }
    M5.Lcd.setCursor(0,LCD_row * 8);            // 液晶描画位置をLCD_row行目に
    if(mode != NCIR) M5.Lcd.printf("ToF=%.0fcm ",Dist/10);      // 測距結果を表示
    if(mode != TOF)  M5.Lcd.printf("Te=%.1f ",Tenv);            // 環境温度を表示
    if(mode == Area){
        M5.Lcd.printf("Ts=%.1f(%.0fcm2) ",Tsen, Ssen / 100);    // 測定温度を表示
        M5.Lcd.printf("To=%.1f(%.0fcm2) ",TobjAra, Sobj / 100); // 物体温度を表示
    }else{
        if(mode != TOF) M5.Lcd.printf("Ts=%.1f ",Tsen);         // 測定温度を表示
        if(mode == Prop)M5.Lcd.printf("To=%.1f ",TobjPrp);      // 物体温度を表示
    }
    LCD_row++;                                  // 行数に1を加算する
    if(LCD_row > 27) LCD_row = 22;              // 最下行まで来たら先頭行へ
    M5.Lcd.fillRect(0, LCD_row * 8, 320, 8, 0); // 描画位置の文字を消去(0=黒)
}
