# 詳細セットアップガイド

ここでは LoRabbit ライブラリや各種サンプルプログラム、サンプルアプリケーションを動作させるための技術情報を記述しています。

# 外部ハードウェアとの接続

EK-RA8D1、RMC-RA4M1 における外部ハードウェアとの接続について説明します。他のボードを利用して動作させたい場合、こちらの説明を参照しつつボードの仕様に合わせて接続するようにしてください。

## EK-RA8D1 Pmod2 コネクタ図

参考として EK-RA8D1 Pmod2 コネクタ図の画像を示します。

![Pmod2 コネクタ](../assets/EK-RA8D1_Pmod2.png)

## RMC-RA4M1 ピン配置

参考として RMC-RA4M1 ピン配置図の画像を示します。

![RMC-RA4M1](../assets/RMC-RA4M1.png)

## LoRa モジュール

LoRabbit ライブラリ、各種サンプルプログラム、サンプルアプリケーションほぼすべてにおいて LoRa モジュールとの接続が必要です。 (heatshrink を使ったサンプルプログラムを除く)

対応モジュールについては下記になります。

- LoRa モジュール
  - CLEALINK 社製 E220-900T22S
  - 作者の手元では [E220-900T22S(JP) 用評価ボード][lora-ev-link] を用いて動作確認を行っています
  - 他のモジュール (E220-900T22S(JP)R2、E220-900T22L(JP)) につきましてはインターフェース仕様が共通のため動作すると思われますが、未確認です

### EK-RA8D1 における接続

下記のように接続します。

| EK-RA8D1 Pmod 2 コネクタ| E220-900T22S(JP) 用評価ボード |
|---|---|
| Pin 2: TXD2  | RxD |
| Pin 3: RXD2  | TxD |
| Pin 7: INT   | AUX |
| Pin 9: GPIO  | M0  |
| Pin 10: GPIO | M1  |
| Pin 11: GND  | GND |
| Pin 12: VCC  | VCC |

### RMC-RA4M1 における接続

下記のように接続します。

| RMC-RA4M1 | E220-900T22S(JP) 用評価ボード |
|---|---|
| P101: TxD3   | RxD |
| P102: RxD3   | TxD |
| P105: INT    | AUX |
| P303: GPIO   | M0  |
| P304: GPIO   | M1  |
| GND          | GND |
| +3.3V        | VCC |

なお、RMC-RA4M1 は IO レベルが 5V、E220-900T22S(JP) 用評価ボードが IO レベルが 3.3V のため、接続の際には間にレベルシフタ回路などを入れるようにして下さい。作者の手元では [4ビット双方向ロジックレベル変換モジュール][BSS138-link] を2つ使ってレベル変換を実施しています。M0、M1ピンについては速度がネックになる処理はないため、プルアップ抵抗とダイオードによる回路でもレベル変換が可能です。

## SPI カメラ

SPI カメラはサンプルアプリケーションの remote_camera_capture におけるRMC-RA4M1 側プロジェクトで必要です。

SPI カメラは下記の製品を想定しています。

- [Arducam MEGA SPI Camera][MEGA-SPI-Camera-link]
  - 作者の手元では [Mega 5MP SPI Camera Module][Mega-5MP-SPI-Camera-link] を用いて動作確認を行っています
  - Arducam MEGA SPI Camera シリーズなら SPI のコマンド互換性があるので、他のモジュールでも OK です

### RMC-RA4M1 における接続

下記のように接続します。

| RMC-RA4M1    | Mega 5MP SPI Camera Module |
|---|---|
| P410: MISOA  | MISO |
| P411: MOSIA  | MOSI |
| P412: RSPCKA | SCK  |
| P413: GPIO   | CS   |
| GND          | GND |
| +3.3V        | VCC |

## プッシュボタン

プッシュボタンはサンプルアプリケーションの comm_log_collector における、
RMC-RA4M1 側プロジェクトで必要です。

作者は [DFRobot の Digital Push Button][DFR0029-link] を用いて動作確認を行っていますが、他の製品も利用可能です。ただし、立ち上がりエッジが Rising か Falling かは確認して下さい。プロジェクトの FSP 設定では Rising Edge で設定しています。

### RMC-RA4M1 における接続

下記のように接続します。

| RMC-RA4M1    | Mega 5MP SPI Camera Module |
|---|---|
| P104: IRQ01  | Input |
| GND          | GND   |
| +3.3V        | VCC   |

# FSP (Flexible Software Package) 設定

プロジェクトにおける FSP 設定について説明します。

## LoRa モジュール通信用 UART

| Group |設定項目      | 値 |
|---|---|---|
| General    | Data Bits    | 8bits |
| General    | Parity       | None  |
| General    | Stop Bits    | 1bit  |
| Baud       | Baud Rate    | 9600  |
| Interrupts | Callback     | 必ず指定すること |

起動時、設定した UART インスタンスを使ってオープンしてください。

```
// LoRa用UARTモジュールを初期化
g_uart2.p_api->open(g_uart2.p_ctrl, g_uart2.p_cfg);
```

また、割り込みの Callback は指定した上で、下記のように LoRabbit ライブラリのハンドラ関数を呼び出して下さい。

```
void g_uart_callback(uart_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_UartCallbackHandler(&s_lora_handle, p_args);
}
```

## LoRa モジュール制御信号(AUX) ピン用 ICU

|設定項目      | 値 |
|---|---|
| Trigger      | Both Edges |
| Callback     | 必ず指定すること  |

起動時、設定した IRQ インスタンスをオープンした上で、割り込みを有効化してください。

```
p_irq1->p_api->open(p_irq1->p_ctrl, p_irq1->p_cfg);
p_irq1->p_api->enable(p_irq1->p_ctrl);
```

割り込みの Callback は指定した上で、下記のように LoRabbit ライブラリのハンドラ関数を呼び出して下さい。

```
void g_irq_callback(external_irq_callback_args_t *p_args) {
    // ライブラリ提供のハンドラを呼び出し、処理を委譲する
    LoRabbit_AuxCallbackHandler(&s_lora_handle, p_args);
}
```

## LoRa モジュール M0, M1 ピン用 GPIO

|設定項目 | 値 |
|---|---|
| Mode     | Output mode (Initial Low) |

## SPI カメラ用 SPI

|設定項目 | 値 |
|---|---|
| Receive Interrupt Priority           | Priority 12       |
| Transfer Complete Interrupt Priority | Priority 12       |
| Operating Mode                       | Master            |
| Callback                             | spi_master_handle |
| Bitrate                              | 4000000           |

Bitrate は更に高速でも動作する可能性がありますが、MCU との接続ケーブル長を短く構成する必要があります。

## SPI カメラ CS 用 GPIO

|設定項目 | 値 |
|---|---|
| Mode     | Output mode (Initial High) |

[lora-ev-link]: https://dragon-torch.tech/rf-modules/lora/
[MEGA-SPI-Camera-link]: https://docs.arducam.com/Arduino-SPI-camera/MEGA-SPI/MEGA-SPI-Camera/
[Mega-5MP-SPI-Camera-link]: https://www.arducam.com/presale-mega-5mp-color-rolling-shutter-camera-module-with-autofocus-lens-for-any-microcontroller.html
[BSS138-link]: https://akizukidenshi.com/catalog/g/g113837/
[DFR0029-link]: https://wiki.dfrobot.com/DFRobot_Digital_Push_Button_SKU_DFR0029
