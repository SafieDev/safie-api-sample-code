ここではAPIキーによる認証を利用してメディアファイルを作成し、ダウンロードするサンプルコードを掲載しています。

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

以下のコマンドでメディアファイルをダウンロードすることができます  
Safie Developers から取得したAPIキーをオプション `--apikey` もしくは環境変数 `SAFIE_API_KEY`   
対象のデバイスのIDをオプション `--device-id`  
作成するメディアの開始日時を `--start` 終了日時を `--end` に設定して実行します

```
$ poetry run python mediafile_download.py \
  --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \
  --device-id 123456789abcdefg \
  --start 2022-11-12T16:41:00 \
  --end 2022-11-12T16:42:00
  
メディアファイル作成要求APIレスポンス
{
    "request_id": 123456789
}
メディアファイル作成要求取得APIレスポンス
{
    "end": "2022-11-12T07:42:00+00:00",
    "request_id": 123456789,
    "start": "2022-11-12T07:41:00+00:00",
    "state": "AVAILABLE",
    "url": "https://openapi.safie.link/v2/devices/123456789abcdefg/media_files/requests/123456789/2022-11-12_07:41:00.mp4"
}
メディアファイルをダウンロードしました。 ダウンロードファイル: /path/to/123456789.mp4
```

以下のコマンドで詳細を確認することができます

```shell
$ poetry run python mediafile_download.py --help
Usage: mediafile_download.py [OPTIONS]

  APIキー認証を利用してメディアファイルのダウンロードを行います

  - メディアファイル作成要求
  - メディアファイル作成状況の確認(30秒間隔で最大10回)
  - メディアファイルダウンロード

Options:
  --apikey TEXT                APIキー  [required]
  --device-id TEXT             デバイスID  [required]
  --start [%Y-%m-%dT%H:%M:%S]  作成するメディアの開始日時(ex. 2022-11-12T16:41:00)
                               JSTの日時を指定してください 作成可能なファイルの録画時間は1分から10分です
                               [required]
  --end [%Y-%m-%dT%H:%M:%S]    作成するメディアの終了日時(ex. 2022-11-12T16:42:00)
                               JSTの日時を指定してください 作成可能なファイルの録画時間は1分から10分です
                               [required]
  --help                       Show this message and exit.
```

# C++ 実装
## 必須要件
- C++コンパイラ
- CMake >=3.13
- pkg-config
- libcurl, cJSON

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

   build/mediafile-download に成果物が配置されます。

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

   build/mediafile-download に成果物が配置されます。

## ビルド手順 (Windows)
WSL2を使用して上記Ubuntuの手順をご利用ください。

## 実行方法
```sh
build/mediafile-download\
  --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX \
  --device-id 123456789abcdefg \
  --start 2022-11-12T16:41:00 \
  --end 2022-11-12T16:42:00
```
