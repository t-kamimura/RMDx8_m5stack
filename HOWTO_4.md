# RMD-X8をM5 Stackで動かす(4)

前回までで，基本的な使い方は全て確認しました．
今回は，クラスを使ってRMD-X8を簡単に制御する方法を確認しましょう．

## RMD-X8_M5クラス

https://github.com/bump5236/RMDx8Arduino
をもとに，M5 Stack用に改良したクラスが`RMD-X8_M5`クラスです．
RMD-X8を制御するために必要なメンバ変数と，メソッドが便利な形で用意されています．

メンバ
|  名前  |  型  |  説明  |
| ---- | ---- | ---- |
| `posKp`	        | uint8_t	| 位置制御のPゲイン	|
| `posKi`	        | uint8_t	| 位置制御のIゲイン	|
| `velKp`	        | uint8_t	| 速度制御のPゲイン	|
| `velKi`	        | uint8_t	| 速度制御のIゲイン	|
| `curKp`	        | uint8_t	| 電流制御のPゲイン	|
| `curKi`	        | uint8_t	| 電流制御のIゲイン	|
| `MOTOR_ADDRESS` | uint16_t	| モーターのアドレス	|
| `tempreture`	| int8_t	| 読み取ったモーターの温度を保持しておく	|
| `present_current` | int16_t	| 読み取った現在の電流値を保持しておく	|
| `present_velocity` | int16_t	| 読み取った現在の速度を保持しておく	|
| `present_pos` | int64_t	| 読み取った現在の位置を保持しておく	|
| `present_angle` | int32_t	| 読み取った現在の角度を保持しておく	|

メソッド
|  名前  |  型  |
| ---- | ---- |
| `canSetup`	        | CANの接続を確立する．`Setup()`内で使用	|
| `readPosition()`	        | 現在の位置を取得	|
| `writeCurrent(int16_t current)`	        | 電流値を指定し，電流制御	|
| `writePosition(int32_t position)`	        | 位置を指定し，位置制御 |
| `stop()`	        | モーターのトルクをOFFにする |


他にも使用頻度の低いものが用意されています．必要に応じて`RMDX8_M5.h`や`RMDX8_M5.cpp`を読んでみましょう．

## RMD-X8クラスを使用する準備

https://github.com/t-kamimura/RMDx8_m5stack に`RMD-X8_M5`フォルダがあります．これを`documents/Arduino/Libraries`フォルダにコピーしましょう．

## RMD-X8クラスを用いたプログラム

サンプルプログラムは`RMD_torsionSpringTest`です．モーターの電流制御によって，回転バネのように振る舞わせるプログラムです．
詳細を見ていきましょう．

```
#include <RMDX8_M5.h>
```
`RMDX8_M5`クラスをインクルードして使えるようにします．

```
RMDX8_M5 myMotor(CAN0, MOTOR_ADDRESS);
```
CANとモーターのアドレスを指定し，`RMD-X8_M5`クラスから`myMotor`インスタンスを生成します．
**クラス**と**インスタンスの**関係については，「MATLAB講習　オブジェクト指向のみ」のビデオ https://youtu.be/G4RlJvAidy8 を参照してください．

```
myMotor.readPosition();
initial_pos = myMotor.present_angle;
```
`Setup()`内では，`readPosition`メソッドを利用してモーターが動き始める前の位置を取得しています．これは開始位置をバネの釣り合い位置とするために行います．
開始位置を目標位置とすることで，**プログラムの開始時に大きなトルクが加えられることなく，安全に実験を行う**ことができます．

```
// read multi turn angle
previous_pos = myMotor.present_angle;
myMotor.readPosition();
delay(1);

present_vel = (myMotor.present_angle - previous_pos)*1000/LOOPTIME;
int32_t target_cur = - KP*(myMotor.present_angle - initial_pos) - KD * present_vel;

myMotor.writeCurrent(target_cur);
delay(1);
```
`loop()`内では，現在の位置と速度を計算し，そこからPD制御によって電流の指令値`target_cur`を決定しています．これは前回と同じです．
電流の指令値が決定したら，`writeCurrent`メソッドで電流指令を行います．

```
// finishing
myMotor.writePosition(0);
delay(2000);
// stop command
myMotor.stop();
delay(500);
SERIAL.println("Program finish!");
```
終了処理として，モーター位置をゼロに戻し，`stop`メソッドを使用してモーターのトルクを切ります．

## まとめ

以上で，RMD-X8をM5 Stackで使用するためのチュートリアルは終了です．
クラスを使いこなせば様々なプログラムが簡単に作れますので，いろいろ試してみましょう．