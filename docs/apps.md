# サンプルアプリケーションについて

[apps][apps-link] には LoRabbit ライブラリを利用したアプリケーションのプロジェクトが置いてあります。

# remote_camera_capture

remote_camera_capture は EK-RA8D1 と RMC-RA4M1 による LoRa 通信を応用したアプリケーションです。EK-RA8D1 からのリクエストに応じて RMC-RA4M1 は接続されているカメラで撮影を行い、その画像を LoRa 通信で送信します。EK-RA8D1 側で画像データを受信すると、接続されている LCD に表示します。

- 下記は動作中の EK-RA8D1 側 LCD の動画になります。
  - https://youtu.be/782smsLdB8Y

## ra4m1_remote_camera_capture

- RMC-RA4M1用のプロジェクト
- 起動すると LoRabbit_ReceiveFrame 関数で EK-RA8D1 からのリクエストを待つ
- リクエストが届くと、接続されているカメラで解像度 96x96 で撮影を行う
- MCU の RAM が少ないため、更に 96x96 のデータを 32x32 に縮小する
- その 32x32 の画像データ (RGB565フォーマット) を LoRabbit_SendData 関数で大容量送信する
- 実行には LoRa モジュールおよび SPI カメラとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra8d1_remote_camera_capture

- EK-RA8D1用のプロジェクト
- 起動するとボード上の S2 ボタンの押下を待つ
- ボタンを押されるとカメラ撮影の要求パケットを LoRabbit_SendFrame 関数で送信する
- 送信後は LoRabbit_ReceiveData 関数で大容量受信待ちを行う
- 受け取ったら、MIPIグラフィックス拡張ボードのLCDに解像度 32x32 の画像データをを8倍に拡大して表示する(256x256)
- 実行には LoRa モジュールとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください
- LCDへの表示に関しては [mtk3bsp2_samples][mtk3bsp2_samples-link] リポジトリの prj_ekra8d1_lcd に含まれている tglib を利用しています。ただし、下記の修正を行っています
  - RGB888 から RGB565サポートに変更 (カメラバッファが RGB565 で直接書けるため)
  - 文字列描画関数を追加 (等倍、拡大)
  - RGB565 バッファの描画関数を追加 (等倍、拡大)

# comm_log_collector

comm_log_collector は EK-RA8D1 と RMC-RA4M1 による LoRa 通信を応用したアプリケーションです。LoRaの設定パラメータを変更しつつデータを複数回送信した後、通信履歴を表示します。機械学習のためのデータ収集に利用できます。

## ra4m1_comm_log_collector

- RMC-RA4M1用のプロジェクト
- 起動すると接続されたボタンの押下を待ちつつ、LoRabbit_ReceiveData 関数で受信も待つ
- ボタンが押されると LoRa の設定パラメータを変更する
- データを受信しても何もしない
  - ライブラリ内部で ACK を返しており、それが目的です
- 実行には LoRa モジュールおよびボタンとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra8d1_comm_log_collector

- EK-RA8D1用のプロジェクト
- 起動するとボード上の S1、または S2 ボタンの押下を待つ
- S1 ボタンが押されると LoRa の設定パラメータを変更する
  - ra4m1_comm_log_collector 側と合わせる必要があります
- S2 ボタンが押されると、乱数で決定したパケットを LoRabbit_SendData 関数で複数回送信する
- 送信後、LoRabbit_ExportHistoryCSV 関数で通信履歴を CSV 形式で表示する
- 実行には LoRa モジュールおよびボタンとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

# ai_adr

ai_adr は EK-RA8D1 と RMC-RA4M1 による LoRa 通信を応用したアプリケーションです。comm_log_collector で収集したログで機械学習した AI 推論モデルからおすすめされた LoRa 通信パラメータに変更します。いわゆる AI による ADR (Adaptive Data Rate) を行っています。

## ra4m1_ai_adr

- RMC-RA4M1用のプロジェクト
- 起動すると接続された　LoRabbit_ReceiveData 関数でデータの受信を待つ
- 設定変更要求が来たらパースしたパラメータで LoRa モジュールを再設定する
- 実行には LoRa モジュールおよびボタンとの接続が必要です。詳細については [詳細セットアップガイド][setup-link] をご参照ください

## ra8d1_ai_adr

- EK-RA8D1用のプロジェクト
- 起動するとボード上の S1、または S2 ボタンの押下を待つ
- S2 ボタンが押されると、テストデータを LoRabbit_SendData 関数で送信
- S1 ボタンが押されると、最新の通信履歴を引数に推論を実施し、適切なパラメータ(データレートと送信電力)を受け取り、それをパケットに格納し設定変更要求を LoRabbit_SendData 関数で送信
- 送信後は自身の LoRa モジュールもおすすめされたパラメータで再設定する

[apps-link]: https://github.com/men100/LoRabbit/tree/main/apps
[mtk3bsp2_samples-link]: https://github.com/tron-forum/mtk3bsp2_samples/tree/main
[setup-link]: setup.md
