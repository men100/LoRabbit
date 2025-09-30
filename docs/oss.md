# 利用している OSS について

こちらでは利用している OSS について記述しています。

# μT-Kernel 3.0

LoRabbitライブラリおよびサンプルプログラム、サンプルアプリケーションで利用しています。

- Git Repository: https://github.com/tron-forum/mtkernel_3/
- Lisence: T-License2.2

# μT-Kernel 3.0 BSP2 (Board Support Package)

LoRabbitライブラリおよびサンプルプログラム、サンプルアプリケーションで利用しています。

- Git Repository: https://github.com/tron-forum/mtk3_bsp2
- Lisence: T-License2.2

# heatshrink

LoRabbitライブラリで利用しています。

- Git Repository: https://github.com/atomicobject/heatshrink
- Lisence: ISC Lisence

なお、LoRabbit 向けには以下に fork したものを用意し、修正しています。ビルドの際はこちらをご利用ください。

- https://github.com/men100/heatshrink/

# Arducam_Mega

サンプルアプリケーションで利用しています。

- Git Repository: https://github.com/ArduCAM/Arducam_Mega
- Lisence: MIT Lisence

なお、LoRabbit 向けには以下に fork したものを用意し、修正しています。ビルドの際はこちらをご利用ください。

- https://github.com/men100/Arducam_Mega/
  - なお、ライブラリについては [Renesas 用 example プロジェクトのもの][libcamera-link] を修正して利用しています

# loar_adr (e-AI から出力された Renesas RA で利用できるソースコード)

LoRabbitライブラリで利用しています。

- Git Repository: なし
- Lisence: Production OEM Lisence
  - http://www.renesas.com/disclaimer

[libcamera-link]: https://github.com/men100/Arducam_Mega/tree/53ed4a3707bda517caf5044dd4fad433ffea0cd4/examples/renesas/EK-RA6M4_arducam_mega/src/libcamera
