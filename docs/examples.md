# サンプルプログラムについて

[examples][examples-link] には LoRabbit ライブラリの各種機能確認用のサンプルプログラムのプロジェクトが置いてあります。

## ra8d1_send

- EK-RA8D1用のプロジェクト
- 5秒おきに LoRabbit_SendFrame 関数による LoRa のパケット送信を行います
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra8d1_recv

- EK-RA8D1用のプロジェクト
- 起動後、LoRabbit_ReceiveFrame 関数によるパケット受信待ちになり、パケットを受信したらそのパケットのメッセージ長、RSSI、メッセージを表示します
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra8d1_heatshrink

- EK-RA8D1用のプロジェクト
- heatshrink を使って、バッファの圧縮と伸長を行います

## ra8d1_recv_compressed_data

- EK-RA8D1用のプロジェクト
- 起動後、LoRabbit_ReceiveCompressedData 関数によるパケット受信待ちになり、パケットを受信したらそのパケットのメッセージ長、RSSI、メッセージを表示します
  - ライブラリ内部では受信したデータを受け取ったあと、伸張を行っています
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra4m1_send

- RMC-RA4M1用のプロジェクト
- 5秒おきに LoRabbit_SendFrame 関数による LoRa のパケット送信を行います
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra4m1_recv

- RMC-RA4M1用のプロジェクト
- 起動後、LoRabbit_ReceiveFrame 関数によるパケット受信待ちになり、パケットを受信したらそのパケットのメッセージ長、RSSI、メッセージを表示します
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra4m1_heatshrink

- RMC-RA4M1用のプロジェクト
- heatshrink を使って、バッファの圧縮と伸長を行います

## ra4m1_send_compressed_data

- RMC-RA4M1用のプロジェクト
- 5秒おきに LoRabbit_SendCompressedData 関数による LoRa のパケット送信を行います
  - ライブラリ内部では入力データの圧縮を行った上で送信を行っています
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

[examples-link]: https://github.com/men100/LoRabbit/tree/main/examples
[setup-link]: setup.md
