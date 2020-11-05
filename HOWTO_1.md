# RMD-X8をM5 Stackで動かす(1)
Gyems社制のBLDCモータ"RMD-X8"(http://www.gyems.cn/846351.html) をArduinoから駆動する方法について．

## はじめに
Boston Dynamics社SPOTやMIT cheetahなどの四足ロボットが開発され，一部は市場に出回るようになってきました．
この流れに乗ってか，小型・高出力のBLDCサーボが多く販売され始めています．しかも１台50,000円程度と安い．Dynamixel Xシリーズと同じくらいの価格帯ですね．非常に便利なので使ってみましょう．

## 必要なもの
* RMD-X8 (AliExpressで購入)
* 電源装置(48V)
* M5 Stack
* M5 Stack COMMU

電源装置はある程度電流値を確保できるものが良いでしょう．筆者の環境ではTEXIO PSW-720L8を使用しました．

## Arduino IDEの準備
まずは，M5 Stackのプログラミング環境を整えます．筆者は情弱なので，かんたんに書けるようにArduino IDEを使います．
(参考) https://docs.m5stack.com/

1. Arduino IDEをインストールしましょう．
2. Arduino IDEを起動し，「ツール」から「ライブラリを管理」を選択して「ライブラリマネージャ」を呼び出します．
3. 「eps32」を検索し，EPS32関連のライブラリをインストール
4. 「M5」を検索し，M5 Stack関連のライブラリをインストール
5. M5 StackをPCに接続し，「ツール」から「ボード」を展開し，"M5Stack-core-ESP32"を選択します．

次に，COMMUモジュールでCAN通信ができるようにmpc_canライブラリを追加します．
ただし，sparkfun社のArduino用CAN-Bus shieldのライブラリも同名ですので，ここでは"mpc_can_m5"に変更します．
1. "C:\Users\ユーザー名\Documents\Arduino\libraries\M5Stack\examples\Modules\COMMU"の"MCP_CAN_lib.rar"を解凍
2. 生成された"MPC_CAN_lib"フォルダを"C:\Users\ユーザー名\Documents\Arduino\libraries"に移動
3. "mcp_can.h"を"mcp_can_m5.h"に変更
4. "mcp_can.cpp"を"mcp_can_m5.cpp"に変更
5. "mcp_can_m5.h"をエディタで開き，"MCP_CAN"を"MCP_CAN_M5"に置換
6. "mcp_can_m5.cpp"をエディタで開き，"MCP_CAN"を"MCP_CAN_M5"に置換
あるいは， https://github.com/t-kamimura/RMDx8_m5stack をクローンし，以上の変更を済ませた"MPC_CAN_lib"フォルダを抜き出す．

## RMD-X8のCANプロトコルを確認
http://www.gyems.cn/support/download からプロトコルをダウンロードして読みましょう．

動作確認用に，PIDパラメータを問い合わせるデータはこちら．

|  Data[0]  |  Data[1]  |  Data[2]   |  Data[3]  | Data[4]  | Data[5]  |  Data[6]  |  Data[7] |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| 0x30 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 | 0x00 |

これを送ってやると，以下のようなデータが返ってきます．

|  Data[0]  |  Data[1]  |  Data[2]   |  Data[3]  | Data[4]  | Data[5]  |  Data[6]  |  Data[7] |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| 0x30 | 0x00 | anglePID_KP | anglePID_KI | speedPID_KP | speedPID_KI | currentPID_KP | currentPID_KI |

今回は，このデータの送受信までやってみることにしましょう．

## 送受信プログラムを作る

### 下地となるプログラム

サンプルプログラムをベースにしましょう．
Arduino IDEを開き，「ファイル」から「スケッチ例」→「M5Stack」→「Modules」→「COMMU」→「CAN」の"commu_can_receiver"と"commu_can_transmitter"を開きます．
今回は，commu_can_receiverをベースにcommu_can_transmitterを合体させてプログラムを改造していきます．

「ファイル」から「名前を付けて保存」で，"RMD-X8_test"としてファイルを作っておきます．

### 受信プログラムの改造

サンプルプログラムでは，読み込みも書き込みも` void test_can();` という関数名になっています．まずは受信する関数の名前を変更しましょう．

1. 29行目の`void test_can();`を`void read_can();`に変更
2. 81行目の`void test_can(){`を`void read_can(){`に変更
3. 55行目の`test_can();`を`read_can();`に変更

これで読み込み側は完成です．

### 送信プログラムの組み込み

"commu_can_transmitter"から，送信に関する部分を持ってきます．

1. `void read_can();`の下に`void write_can();`を追加
2. "commu_can_transmitter"の`void test_can{~}`（71行目から82行目）を全部コピーし，"RMD-X8_test"の最後の部分にペースト
3. `void test_can{~}`の名前を`void write_can{~}`に変更
4. `void loop{~}`内の`read_can();`のすぐ上に`write_can();`を追加
5. 14行目を`byte data[8] = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};`に変更

これで完成です．

## 動かしてみる

M5 stackにプログラムを書き込み，モーターとつないで電源を投入します．
モーターからデータが返ってきていたら成功です．