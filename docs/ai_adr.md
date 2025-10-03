# AI モデルの作り方

 LoRabbit で利用している AI モデルの作り方を解説します。自分の環境に特化した AI モデルを作りたい場合などに参考にして下さい。

# 1. データ収集

サンプルアプリケーションの comm_log_collector を使い、様々な条件下で通信を行い、通信履歴をCSV形式でPCに保存しました (LoRabbit_ExportHistoryCSVを使用)。

例えば以下のようなデータが取れます。

```c
timestamp_lo,data_size,air_data_rate,transmitting_power,ack_requested,ack_success,last_ack_rssi,total_retries
3355782796,33,0,1,1,1,-92,0
3355784196,163,0,1,1,1,-92,0
3355785736,122,0,1,1,1,-92,0
3355787216,29,0,1,1,1,-92,0
3355788616,120,0,1,1,1,-92,0
```

# 2. 訓練

PythonとTensorFlow/Kerasを使って、収集したCSVデータを学習させました。

下記が訓練し、TensorFlow Lite 形式 (.tflite) で保存している Python コードです。
マイコン向けに 8bit 量子化を実施しています。

```python
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
import numpy as np
import os

# データの読み込みと前処理
CSV_FILE_PATH = 'lora_comm_log_history.csv'

if not os.path.exists(CSV_FILE_PATH):
    print(f"エラー: CSVファイル '{CSV_FILE_PATH}' が見つかりません。")
else:
    df = pd.read_csv(CSV_FILE_PATH)

    # 「直前の通信結果」を特徴量として使えるように、列を1行ずらす
    df['last_ack_rssi'] = df['last_ack_rssi'].shift(1)
    df['last_ack_success'] = df['ack_success'].shift(1)

    # 最初の行には「直前」のデータがない(NaN)ので、その行を削除し、型を変換
    df = df.dropna().astype({'last_ack_rssi': 'float32', 'last_ack_success': 'float32'})

    # 特徴量 (X): AIへの入力 (直前のRSSI, 直前の成功/失敗)
    X = df[['last_ack_rssi', 'last_ack_success']].values

    # 正解ラベル (y): AIに予測させたい出力 (次に使うべきデータレート)
    # LoRaのデータレート(enumの整数値)をカテゴリとして扱う
    y_categorical, class_names = pd.factorize(df['air_data_rate'], sort=True)
    num_classes = len(class_names)
    
    print("--- Data Preprocessing ---")
    print(f"Found {num_classes} unique classes (data rates) to predict.")
    print("Class names (in order):")
    for i, name in enumerate(class_names):
        print(f"  Index {i}: {name}") # この順序がC言語側のマッピングで重要になります
    print("-" * 20)

    # データを訓練用とテスト用に分割 (例: 70%を訓練に、30%を性能評価に)
    X_train, X_test, y_train, y_test = train_test_split(X, y_categorical, test_size=0.3, random_state=42)

    # Kerasでニューラルネットワークモデルを構築
    model = tf.keras.Sequential([
        tf.keras.layers.InputLayer(input_shape=(2,)), # 入力層 (特徴量は2つ)
        tf.keras.layers.Dense(16, activation='relu'), # 中間層 (16ニューロン)
        tf.keras.layers.Dense(8, activation='relu'),  # 中間層 (8ニューロン)
        tf.keras.layers.Dense(num_classes, activation='softmax') # 出力層 (クラス数分のニューロン)
    ])

    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])
    
    model.summary()

    # モデルを訓練
    print("\n--- Starting Model Training ---")
    model.fit(X_train, y_train, epochs=50, validation_split=0.2, verbose=1)
    print("-" * 20)

    # モデルの性能を評価
    loss, accuracy = model.evaluate(X_test, y_test, verbose=0)
    print(f'Keras Model Accuracy on Test Data: {accuracy*100:.2f}%')

    # モデルを8ビット整数量子化されたTensorFlow Lite形式に変換
    print("\n--- Converting to TensorFlow Lite (8-bit quantized) ---")
    
    # 代表的なデータセットを生成する関数を定義
    def representative_dataset_gen():
        # 訓練データの一部を入力として与える
        for value in X_train:
            yield [value.reshape(1, 2).astype('float32')]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    # 最適化設定
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    # 代表的なデータセットを設定
    converter.representative_dataset = representative_dataset_gen
    # マイコンで一般的な8ビット整数のみを使うように指定
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    # 入出力の型も8ビット整数に指定
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_model_quant = converter.convert()

    # .tfliteファイルとして保存
    with open('lora_adr_model_quant.tflite', 'wb') as f:
        f.write(tflite_model_quant)

    print("Successfully saved 8-bit quantized model to 'lora_adr_model_quant.tflite'")
```

# 3. 変換

Renesas 製 e-AI トランスレータを使うと、.tflite を RA マイコン向けの C 言語コードに変換してくれるので、今回利用しました。

変換されたコードは [lora_adr][lora_adr-link] にあります。

e-AI トランスレータのダウンロード、マニュアルについては [マイクロコンピュータ向け e-AI開発環境 | Renesas ルネサス][e-ai-link] をご参照下さい。

# 4. 組み込み

モデルのコードを LoRabbit ライブラリで利用できるように組み込み作業を行いました。以下の点がポイントです。

## 4-1. 推論に必要な関数は dnn_compute_lora_adr

基本的には生成された dnn_compute_xxx (xxx は e-AI でユーザが指定できます) を呼び出せば OK です。

## 4-2. ヘッダに定義されている変数の対処

e-AI で生成されたヘッダファイルは複数のコードからの include を想定していないようで、ヘッダファイルに配列や構造体の定義が書かれていました。そのため、LoRabbit から取り込もうとするとコンパイルエラーが出てしまうため、ヘッダファイルの方を extern 宣言化し、[定義を LoRabbit_ai_adr.c で行うように変更しました。][header_fix-link]

以上を行うことで、AIモデルを LoRabbit ライブラリで利用できるようになりました。

[lora_adr-link]: https://github.com/men100/LoRabbit/tree/main/src/lora_adr
[e-ai-link]:https://www.renesas.com/ja/key-technologies/artificial-intelligence/e-ai/e-ai-development-environment-microcontrollers?srsltid=AfmBOorpJRolVQ0-g_NSoXUsVOx3j0wtBe3rsMMa9xtKTOiV2IrCWOuw
[header_fix-link]: https://github.com/men100/LoRabbit/blob/main/src/LoRabbit/LoRabbit_ai_adr.c#L32-L53
