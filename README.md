# RMDx8_m5stack

RMD-X8 driving programs using m5stack with commu. extension board

[NOTICE]

Before compiling, copy "MCP_CAN_lib" folder to the "Arduino > libraries" folder.

[注意]

コンパイルする前に，"MCP_CAN_lib" フォルダをまるごと "Arduino > libraries" フォルダにコピーしてください．

## CAN通信の予備実験

### RMD_CANTest

M5 StackとRMD-X8の通信実験のためのプログラム．
RMD-X8のPIDゲインを調べるだけ．

### RMD_moveTest

`RMD_CAN`をもとに作った，TestRMD-X8の位置制御プログラム．

## RMD-X8の動作実験

### RMD_ReadPosTest

絶対角度読み込みのテストプログラム．サーボホーンの角度をdeg単位で返す．

### RMD_PIDGainTest

位置制御指令のテストプログラム．位置・速度・電流それぞれに対して設定されたPIゲインを変更し，どのようなバネ特性を示すか調べる．

### RMD_CurrentTest

電流制御のテストプログラム．現在角度を読み込んで，PD制御を行う．

### RMD_FreqResp

位置指令による周波数応答を調べるためのプログラム．

## クラス化

未実装．

## RTOS

高速化のためにRTOSを試す．

### RTOS_test

マルチタスク実験．画面に2種類の文字列を異なる周期で写すプログラム．
周期の公倍数で同時に画面描画を行うタイミングで表示がおかしくなる．

### RT_ReadWrite

マルチタスクでRMD-X8を動かしたい．
CAN通信で書き込み・読み込みを行うタスクと，受け取ったデータを処理して次の指令値を作ったりするタスクに分散させる．
変数に対して同時書き込み・読み込みが発生するためエラーが出る．未解決．