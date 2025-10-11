# AI モデルの作り方

 LoRabbit で利用している AI モデルの作り方を解説します。自分の環境に特化した AI モデルを作りたい場合などに参考にして下さい。

# 1. そもそもAI-ADRとは？ 
ADR (Adaptive Data Rate) とは、無線通信において、通信環境に応じてデータレート (通信速度) や送信電力を動的に調整する技術です。これにより、通信の信頼性を確保しつつ、消費電力や通信時間を最適化できます。

LoRabbit ライブラリが実装するAI-ADRは、この調整をAI（機械学習モデル）の推論によって行います。過去の通信履歴（電波の強さや成功/失敗など）をAIが学習し、その結果に基づいて「次に最も成功しやすく、効率的な通信パラメータは何か？」を予測・推奨します。

この機能により、ユーザーは環境ごとに手動でパラメータを調整する手間なく、安定した通信を維持しやすくなります。

# 2. AIモデルの仕組み：入力と出力

本ライブラリで使われているAIモデルは、シンプルな構造になっています。

- AIへの入力 (特徴量): 直前の通信における以下の2つの情報です。
  - 最後のACK受信時の電波の強さ (RSSI)
  - 最後のACK受信が成功したか否か

- AIからの出力 (予測結果): 上記の入力情報を基に、次に使うべき最も効果的な"データレート (Air Data Rate)"を推奨値として出力します。

例えば「電波は強いのに通信に失敗した」という状況であれば、AIは「データレートが高すぎるのかもしれない」と判断し、より低いデータレートを推奨する、といった学習を行います。

# 3. データ収集

## 3-1. comm_log_collector の利用

サンプルアプリケーションの comm_log_collector を使い、様々な条件下で通信を行い、通信履歴をCSV形式でPCに保存します (LoRabbit_ExportHistoryCSVを使用)。

例えば以下のようなデータが取れます。

```c
timestamp_lo,data_size,air_data_rate,transmitting_power,ack_requested,ack_success,last_ack_rssi,total_retries
3355782796,33,0,1,1,1,-92,0
3355784196,163,0,1,1,1,-92,0
3355785736,122,0,1,1,1,-92,0
3355787216,29,0,1,1,1,-92,0
3355788616,120,0,1,1,1,-92,0
```

## 3-2. 良いモデルを作るためのヒント

AIの性能は学習データの質と量に大きく依存します。独自の環境に最適化されたモデルを作りたい場合、以下の点を意識してデータを収集することをお勧めします。

- 多様な環境で収集する: 電波が通りやすい見通しの良い場所、壁や障害物がある場所、距離が近い場合・遠い場合など、実際に使用する可能性のある様々な環境でデータを集めましょう。
- 多様な設定を試す: comm_log_collectorを使い、様々なデータレートや送信電力の組み合わせで通信ログを記録してください。これにより、AIが幅広い状況を学習できます。
- 十分なデータ量: 明確な基準はありませんが、まずは数千件の通信ログを目標にすると良いでしょう。データが多ければ多いほど、モデルの予測精度は向上する傾向にあります。

# 4. 訓練

## 4-1. Python 

PC上にインストールしたPythonとTensorFlow/Kerasを使って、収集したCSVデータを学習させましょう。

下記が訓練し、TensorFlow Lite 形式 (.tflite) で保存している内容の Python コードです。

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

## 4-2. コードの解説

- 「直前の通信結果」を入力データとして使いたいので、RSSIと成功/失敗の列を1行下にずらします (16-17行目)

```python
df['last_ack_rssi'] = df['last_ack_rssi'].shift(1)
df['last_ack_success'] = df['ack_success'].shift(1)
```

- 最初の行には「直前」のデータがないため、この行を削除します (20行目)

```python
df = df.dropna().astype({'last_ack_rssi': 'float32', 'last_ack_success': 'float32'})
```

- AIへの入力 (X) と、AIに予測させたい正解 (y) にデータを分けます (23-27行目)

```python
X = df[['last_ack_rssi', 'last_ack_success']].values
y_categorical, class_names = pd.factorize(df['air_data_rate'], sort=True)
```

- Kerasを使ってAIモデルの形を定義します (41-46行目)
  - 入力層(2) -> 中間層(16) -> 中間層(8) -> 出力層(クラス数) という構造です

```python
model = tf.keras.Sequential([
    tf.keras.layers.InputLayer(input_shape=(2,)), # 入力層 (特徴量は2つ)
    tf.keras.layers.Dense(16, activation='relu'), # 中間層 (16ニューロン)
    tf.keras.layers.Dense(8, activation='relu'),  # 中間層 (8ニューロン)
    tf.keras.layers.Dense(num_classes, activation='softmax') # 出力層 (クラス数分のニューロン)
])
```

- 準備したデータを使ってモデルを学習させます (56行目)

```python
model.fit(X_train, y_train, epochs=50, validation_split=0.2, verbose=1)
```

- マイコンで高速に動作させるために、モデルを軽量な形式(.tflite)に変換します
  - 8bit量子化は、モデルのサイズを削減し、計算を軽くするための最適化です

```python
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
```

# 5. 変換

Renesas 製 e-AI トランスレータを使うと、.tflite を RA マイコン向けの C 言語コードに変換してくれるので、利用するのも良い手段だと思います。

変換されたコードは [lora_adr][lora_adr-link] にあります。

e-AI トランスレータのダウンロード、マニュアルについては [マイクロコンピュータ向け e-AI開発環境 | Renesas ルネサス][e-ai-link] をご参照下さい。

# 6. 組み込み

モデルのコードを LoRabbit ライブラリで利用できるように下記のような組み込み作業を行いました。

## 6-1. 推論に必要な関数は dnn_compute_lora_adr

基本的には生成された dnn_compute_xxx (xxx は e-AI でユーザが指定できます) を呼び出せば OK です。

## 6-2. ヘッダに定義されている変数の対処

e-AI で生成されたヘッダファイルは複数のコードからの include を想定していないようで、ヘッダファイルに配列や構造体の定義が書かれていました。そのため、LoRabbit から取り込もうとするとコンパイルエラーが出てしまうため、ヘッダファイルの方を extern 宣言化し、[定義を LoRabbit_ai_adr.c で行うように変更しました。][header_fix-link]

以上を行うことで、AIモデルを LoRabbit ライブラリで利用できるようになりました。

[lora_adr-link]: https://github.com/men100/LoRabbit/tree/main/src/lora_adr
[e-ai-link]:https://www.renesas.com/ja/key-technologies/artificial-intelligence/e-ai/e-ai-development-environment-microcontrollers?srsltid=AfmBOorpJRolVQ0-g_NSoXUsVOx3j0wtBe3rsMMa9xtKTOiV2IrCWOuw
[header_fix-link]: https://github.com/men100/LoRabbit/blob/main/src/LoRabbit/LoRabbit_ai_adr.c#L32-L53
