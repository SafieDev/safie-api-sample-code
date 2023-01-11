ここではAPIキーによる認証を利用してライブストリーミング映像をダウンロードするサンプルコードを掲載しています。

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

以下のコマンドでストリーミング映像をダウンロードすることができます(1分毎にファイルを作成し保存)  
Safie Developers から取得したAPIキーをオプション `--apikey` もしくは環境変数 `SAFIE_API_KEY`   
対象のデバイスのIDをオプション `--device-id` に設定して実行します  

```
$ poetry run python streaming-download.py \
  --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \
  --device-id 123456789abcdefg
```

以下のコマンドで詳細を確認することができます

```shell
$ poetry run python streaming-download.py --help
Usage: streaming-download.py [OPTIONS]

Options:
  --apikey TEXT     APIキー  [required]
  --device-id TEXT  デバイスID  [required]
  --help            Show this message and exit.
```

# C++ 実装
## 必須要件
- C++コンパイラ
- CMake >=3.13
- pkg-config
- FFmpeg 4.xまたは5.x

## ビルド手順 (Ubuntu 22.04)
1. 必要なパッケージをインストールします
   ```sh
   apt-get install -y g++ cmake pkg-config libavformat-dev libavcodec-dev libavutil-dev
   ```

2. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/streaming-download に成果物が配置されます。

## ビルド手順 (macOS)
1. AppleのサイトよりXcodeコマンドラインツールをインストールします

2. Homebrewにより下記パッケージをインストールします
   ```sh
   brew install pkg-config cmake ffmpeg
   ```

3. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/streaming-download に成果物が配置されます。

## ビルド手順 (Windows)
WSL2を使用して上記Ubuntuの手順をご利用ください。

## 実行方法
```sh
build/streaming-download\
  --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \
  --device-id 123456789abcdefg
```
