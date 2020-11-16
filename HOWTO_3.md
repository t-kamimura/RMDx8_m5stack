# RMD-X8をM5 Stackで動かす(3)

前回は角度の読み込みとモーターの位置制御ができるようになりました．
今回は，読み込んだ角度をもとにフィードバック制御を行ってみましょう．
サンプルプログラムは https://github.com/t-kamimura/RMDx8_m5stack にあります．該当するプログラム名はそれぞれの詳細説明の中にあります．

## PD制御

ここでは，最も簡単なフィードバック制御の一つである，**PD制御**を行います．

モーターをある目標角度$\theta_d$に持っていきたいとき，モーターが出すトルク$\tau$を
$$
\tau = K_P (\theta - \theta_d) + K_D\dot{\theta}
$$
とします．ここで，$\theta$はモーターの現在角度，$\dot{\theta}$はモーターの現在角速度です．$K_P$，$K_D$はそれぞれ**比例ゲイン**，**微分ゲイン**と呼ばれます．
比例項$K_P (\theta - \theta_d)$は，現在の角度が離れているほど大きなトルクを生成します．これによって目標値に速やかに向かうことができます．
ただし，比例項だけでは目標値を通り過ぎてしまう（**オーバーシュート**）ので，粘性摩擦を表す微分項$K_D\dot{\theta}$を導入し，速やかに収束するようにしています．

ただし，RMD-X8はトルクセンサーを内蔵していないため，トルク制御を表す上記の制御則をそのまま実装することはできません．そこで，モーターが出すトルクはモーターに流れている電流値にほぼ比例するという性質を利用し，
$$
I = K_P (\theta - \theta_d) + K_D\dot{\theta}
$$
という制御を考えます．RMD-X8には電流センサーが内蔵されているため，この制御則は実装可能です．

## 電流制御プロトコル

RMD-X8で電流制御を行うときは，以下のコマンドを送信します．

|  指令  |  data[0]  |  data[1]  |  data[2]   |  data[3]  | data[4]  | data[5]  |  data[6]  |  data[7] |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |
|Current control	| 0xA1	|0x00	|0x00	|0x00	|current value low byte	|current value high byte	|0x00	|0x00|

電流指令は`int16_t`型で作成します．指令値は-2000 ~ 2000の間で設定され，電流値としては-32 A ~ 32 Aに相当します．
ただし，RMD-X8の定格電流は4.9 Aなので，安全に駆動できる指令値は-300～300くらいの範囲になります．

CANで送るときは，16ビット(=2バイト)で表された電流指令値を2つのバイトに分割します．

```c
int32_t tgt_cur = 100;  //任意の値
cmd_buf[0] = 0xA1;
cmd_buf[1] = 0x00;
cmd_buf[2] = 0x00;
cmd_buf[3] = 0x00;
cmd_buf[4] = tgt_cur & 0xFF;
cmd_buf[5] = (tgt_cur >> 8) & 0xFF;
cmd_buf[6] = 0x00;
cmd_buf[7] = 0x00;
```

## PD制御の実装

電流制御によるPD制御のサンプルプログラムは`RMD_CurrentTest`です．

まず，前回説明した角度読み取り指令を使って現在の角度を取得します．
このとき，角速度も計算しておきます．

```c
// read multi turn angle
cmd_buf[0] = 0x92;
cmd_buf[1] = 0x00;
cmd_buf[2] = 0x00;
cmd_buf[3] = 0x00;
cmd_buf[4] = 0x00;
cmd_buf[5] = 0x00;
cmd_buf[6] = 0x00;
cmd_buf[7] = 0x00;
write_can();
delay(1);   // ここを入れておかないと，サーボ側の処理が間に合わない
read_can();

// 角度と角速度の計算
pos_buf = angle;    //速度を計算するために前回の値を取っておく

present_pos = 0;
present_pos = reply_buf[1] + (reply_buf[2] << 8) + (reply_buf[3] << 16) + (reply_buf[4] << 24) + (reply_buf[5] << 32) + (reply_buf[6] << 48);

angle = present_pos * 0.01 / 6;             //サーボホーンの角度 [deg]
vel = (angle - pos_buf)/(LOOPTIME*0.001);    //サーボホーンの角速度 [deg/s]
```

`LOOPTIME`はループにかかる時間(ms)を表しています．
速度を計算するためにループの時間は一定で回しておきたいので，マイコン内部の時刻をミリ秒単位で取得する`millis()`を用いて

```c
void loop()
{
    timer[1] = millis();
    /*
    // 諸々の処理
    */
    timer[2] = millis() - timer[1];
    if (timer[2] < LOOPTIME)
    {
        delay(LOOPTIME - timer[2]);
    }
}
```

としておきます．`//諸々の処理`の部分に`LOOPTIME`以上の時間がかかると意味がありませんので注意しましょう．

次に，電流指令を行います．

```c
int32_t tgt_cur = -KP * (angle - TGT_POS) - KD*vel;

// current control command
cmd_buf[0] = 0xA1;
cmd_buf[1] = 0x00;
cmd_buf[2] = 0x00;
cmd_buf[3] = 0x00;
cmd_buf[4] = tgt_cur & 0xFF;
cmd_buf[5] = (tgt_cur >> 8) & 0xFF;
cmd_buf[6] = 0x00;
cmd_buf[7] = 0x00;
write_can();
delay(1);   // ここを入れておかないと，サーボ側の処理が間に合わない
read_can();
```

`TGT_POS`はサーボホーンの目標値(deg)，`KP`，`KD`はそれぞれ比例ゲイン，微分ゲインです．
これで，PD制御の実装ができました．
