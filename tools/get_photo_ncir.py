#!/usr/bin/env python3
# coding: utf-8

################################################################################
# カメラからの配信画像を取得する Python版 [体温計用]
#
################################################################################
# 使用、改変、再配布は自由に行えますが、無保証です。権利情報の改変は不可です。
#                                          Copyright (c) 2016-2021 Wataru KUNINO
################################################################################

DEV_CAM = 'cam_a_5'                                     # 配信デバイス名(カメラ)
DEV_PIR = 'pir_s_5'                                     # 配信デバイス名(人感)
SAVETO  = 'photo'                                       # 保存先フォルダ名
IP_CAM  = None                                          # カメラのIPアドレス
PORT    = 1024                                          # UDPポート番号を1024に

# import pathlib                                        # ファイル・パス用
from os import makedirs                                 # フォルダ作成用
import socket                                           # ソケット通信用
import urllib.request                                   # HTTP通信ライブラリ
import datetime                                         # 年月日・時刻管理
pir  = 0                                                # 測定中=1,測定終了=0
temp = 0.0                                              # 温度値

def cam(ip, filename = 'cam.jpg'):                      # IoTカメラ
    filename = SAVETO + '/' + filename                  # フォルダ名を追加
    url_s = 'http://' + ip                              # アクセス先をurl_sへ
    s = '/cam.jpg'                                      # 文字列変数sにクエリを
    try:
        res = urllib.request.urlopen(url_s + s)         # IoTカメラで撮影を実行
        if res.headers['content-type'].lower().find('image/jpeg') < 0:
            print('Error content-type :',res.headers['content-type'])
            return None                                 # JPEGで無いときにNone
    except urllib.error.URLError:                       # 例外処理発生時
        print('Error urllib :',url_s)                   # エラー表示
        return None                                     # 関数を終了する
    data = res.read()                                   # コンテンツ(JPEG)を読む
    try:
        fp = open(filename, 'wb')                       # 保存用ファイルを開く
    except Exception as e:                              # 例外処理発生時
        print(e)                                        # エラー内容を表示
        return None                                     # 関数を終了する
    fp.write(data)                                      # 写真ファイルを保存する
    fp.close()                                          # ファイルを閉じる
    print('saved file :',filename)                      # 保存ファイルを表示する
    return filename                                     # ファイル名を応答する

print('Get Photo for Python [NCIR]')                    # タイトル表示
# pathlib.Path(SAVETO).mkdir(exist_ok=True)             # フォルダ作成
makedirs(SAVETO, exist_ok=True)                         # フォルダ作成

print('Listening UDP port', PORT, '...')                # ポート番号表示
try:
    sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)# ソケットを作成
    sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)    # オプション
    sock.bind(('', PORT))                               # ソケットに接続
except Exception as e:                                  # 例外処理発生時
    print(e)                                            # エラー内容を表示
    exit()                                              # プログラムの終了
while sock:                                             # 永遠に繰り返す
    try:
        udp, udp_from = sock.recvfrom(128)              # UDPパケットを取得
        udp = udp.decode()                              # UDPデータを文字列に
    except Exception as e:                              # 例外処理発生時
        print(e)                                        # エラー内容を表示
        continue                                        # whileの先頭に戻る
    s=''                                                # 表示用の文字列変数s
    for c in udp:                                       # UDPパケット内
        if ord(c) > ord(' ') and ord(c) <= ord('~'):    # 表示可能文字
            s += c                                      # 文字列sへ追加
    date = datetime.datetime.today()                    # 日時を取得
    date_s = date.strftime('%Y/%m/%d %H:%M:%S')         # 日時を文字列に変換
    print(date_s + ', ', end='')                        # 日時を出力
    print(udp_from[0], end='')                          # 送信元アドレスを出力
    print(', ' + s, flush=True)                         # 受信データを出力
    if s[5] != '_' or s[7] != ',':                      # 指定書式で無い時
        continue                                        # whileの先頭に戻る
    device = s[0:7]                                     # 先頭7文字をデバイス名
    value = s.split(',')                                # CSVデータを分割
    if device == DEV_CAM and IP_CAM is None:            # カメラを発見したとき
        IP_CAM = udp_from[0]                            # カメラIPアドレスを保持
        print('カメラを発見しました IP_CAM =',IP_CAM)   # 発見を表示
    if device == DEV_PIR and len(value) >= 4:           # 体温測定実験機から
        try:
            pir = int(value[1])                         # 測定状態を取得
            temp = float(value[3])                      # 体温を取得
        except ValueError:                              # 数値で無かったとき
            continue                                    # whileの先頭に戻る
        print('pir =', pir, ', temp =', temp, end=', ') # 測定状態と体温を表示
        if IP_CAM is not None:                          # IP_CAMがNoneでは無い時
            date_f = date.strftime('%Y%m%d-%H%M%S')     # ファイル名用の時刻書式
            if temp >= 37.5:                            # 37.5℃以上の時
                filename = 'cam_' + date_f + '.' + value[3] + '.jpg' #ファイル名
                cam(IP_CAM,filename)                    # 撮影(ファイル保存)
            elif pir == 0:                              # 測定終了時
                filename = 'cam_' + date_f + '.jpg'     # ファイル名を生成
                cam(IP_CAM,filename)                    # 撮影(ファイル保存)
            else:                                       # その他のとき
                cam(IP_CAM)                             # 撮影(ファイル上書き)
        else:                                           # カメラが無い時
            print('no IP_CAM')                          # カメラ未登録を表示
        try:
            fp = open(SAVETO + '/log.csv', 'a')         # ログ保存用ファイル開く
        except Exception as e:                          # 例外処理発生時
            print(e)                                    # エラー内容を表示
            continue                                    # whileの先頭に戻る
        log = date_s + ', '+ str(pir) + ', '+ str(temp) # ログを生成
        fp.write(log + '\n')                            # ログを出力
        fp.close()                                      # ファイルを閉じる

'''
$ ./get_photo_ncir.py
Get Photo for Python [NCIR]
Listening UDP port 1024 ...
2021/01/24 10:56:40, 192.168.1.2, cam_a_5,0,http://192.168.1.2/cam.jpg
カメラを発見しました IP_CAM = 192.168.1.2
2021/01/24 10:57:52, 192.168.1.5, pir_s_5,1,0,34.5
pir = 1 , temp = 34.5 , saved file : photo/cam.jpg
2021/01/24 10:57:55, 192.168.1.5, pir_s_5,1,1,36.1
pir = 1 , temp = 36.1 , saved file : photo/cam.jpg
2021/01/24 10:57:57, 192.168.1.5, pir_s_5,0,1,36.1
pir = 0 , temp = 36.1,  saved file : photo/cam_20210124-105757.jpg
2021/01/24 11:11:38, 192.168.1.5, pir_s_5,1,1,36.7
pir = 1 , temp = 36.7, saved file : photo/cam.jpg
2021/01/24 11:11:40, 192.168.1.5, pir_s_5,1,1,38.1
pir = 1 , temp = 38.1, saved file : photo/cam_20210124-111140.38.1.jpg
2021/01/24 11:11:43, 192.168.1.5, pir_s_5,0,1,38.1
pir = 0 , temp = 38.1, saved file : photo/cam_20210124-111143.38.1.jpg
'''
