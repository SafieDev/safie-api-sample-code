ここではAPIキーによる認証を利用した以下の解析タスクを実行するサンプルコードを掲載しています。
1. デバイスから画像取得
2. 画像からイベント判定
3. デバイスに対するイベント登録

# Python 実装

## 必須要件
- Python 3.10
- poetry

## 環境構築

Poetryを用いて実行環境を構築します

```shell
$ poetry install
```

## 実行

以下のコマンドで解析タスクの実行を開始できます

実行する際は以下のオプションもしくは環境変数を指定する必要があります
- `--apikey` (環境変数 `SAFIE_API_KEY`) : Safie Developers から取得したAPIキー
- `--device-id` : 利用するカメラのデバイスID
- `--definition-id` : Safie Developers で登録したイベント定義ID

実行すると、5秒おきに解析タスクを実施されていきます

終了する際は CTRL-C で終了してください


```shell
$ poetry run python get_image_set_flag.py --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX --device-id DDDDDDDDDDDDDDDDDDDD --definition-id ev_EEEEEEEEEEEEEEEE
downloaded latest device image (29587 bytes)
analyze result: score=0.8134851425223426, is_found=False, dead_time=0
event not found
downloaded latest device image (29200 bytes)
analyze result: score=0.39079039713152247, is_found=False, dead_time=-1
event not found
downloaded latest device image (29331 bytes)
analyze result: score=-0.14834041117833938, is_found=False, dead_time=-2
event not found
downloaded latest device image (29300 bytes)
analyze result: score=-0.6400877090181488, is_found=False, dead_time=-3
event not found
downloaded latest device image (29063 bytes)
analyze result: score=-0.9458060430010351, is_found=False, dead_time=-4
event not found
downloaded latest device image (29180 bytes)
analyze result: score=-0.9771801874861765, is_found=False, dead_time=-5
event not found
downloaded latest device image (28951 bytes)
analyze result: score=-0.725403944518527, is_found=False, dead_time=-6
event not found
downloaded latest device image (29266 bytes)
analyze result: score=-0.2618391854190125, is_found=False, dead_time=-7
event not found
downloaded latest device image (29021 bytes)
analyze result: score=0.27853239790292766, is_found=False, dead_time=-8
event not found
downloaded latest device image (29045 bytes)
analyze result: score=0.7357504499566003, is_found=False, dead_time=-9
event not found
downloaded latest device image (28871 bytes)
analyze result: score=0.9806253885342279, is_found=True, dead_time=-10
event found! so posted event
downloaded latest device image (28871 bytes)
analyze result: score=0.9524738659602884, is_found=True, dead_time=3
event found but ignored
...
```

# C++ 実装
## 必須要件
- C++コンパイラ
- CMake >=3.13
- pkg-config
- libcurl
- cJSON

## ビルド手順 (Ubuntu 22.04)
1. 必要なパッケージをインストールします
   ```sh
   apt-get install -y g++ cmake pkg-config libcurl4-openssl-dev libcjson-dev
   ```

2. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/get-image-set-flag に成果物が配置されます。

## ビルド手順 (macOS)
1. AppleのサイトよりXcodeコマンドラインツールをインストールします

2. Homebrewにより下記パッケージをインストールします
   ```sh
   brew install pkg-config cmake curl cjson
   ```

3. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/get-image-set-flag に成果物が配置されます。

## ビルド手順 (Windows)
WSL2を使用して上記Ubuntuの手順をご利用ください。

## 実行方法
```sh
build/get-image-set-flag\
  --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \
  --device-id DDDDDDDDDDDDDDDDDDDD \
  --definition-id ev_EEEEEEEEEEEEEEEE
```
