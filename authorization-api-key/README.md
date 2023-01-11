ここではAPIキーによる認証を利用してデバイス一覧を取得するサンプルコードを掲載しています。

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

以下のコマンドでデバイス一覧をJSON形式で取得することができます
Safie Developers から取得したAPIキーをオプション `--apikey` もしくは環境変数 `SAFIE_API_KEY` に設定して実行します

```
$ poetry run python list_devices.py --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
{
    "count": 1,
    "has_next": false,
    "list": [
        {
            "device_id": "ABCDEFGHIJKLMNOPQRST",
            "model": {
                "description": "sample model"
            },
            "serial": "0123456789",
            "setting": {
                "name": "sample device"
            },
            "status": {
                "video_streaming": true
            }
        }
    ],
    "offset": 0,
    "total": 1
}
```

スクリプトには幾つかのオプションが用意されており、以下のコマンドで詳細を確認することができます

```shell
$ poetry run python list_devices.py --help
Usage: list_devices.py [OPTIONS]

  APIキー認証を利用してデバイス一覧を取得します

Options:
  --apikey TEXT           APIキー  [required]
  --offset INTEGER RANGE  返却するリストのオフセット (デフォルト値: 0)  [0<=x<=10000]
  --limit INTEGER RANGE   返却するリストの最大件数 (デフォルト値: 20  [1<=x<=100]
  --item-id INTEGER       デバイスに付与されているオプションによる絞り込み (具体的な値は
                          https://developers.safie.link/terms-of-api をご参照ください
  --all                   全てのデバイス一覧を取得する (このオプションを指定した場合、他のオプション指定は無視されます)
  --help                  Show this message and exit.
```

# C++実装

## 必須要件
- C++コンパイラ
- CMake >= 3.13
- pkg-config
- libcurl

## ビルド手順 (Ubuntu 22.04)
1. 必要なパッケージをインストールします
   ```sh
   apt-get install -y g++ cmake pkg-config libcurl4-openssl-dev
   ```

2. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/list-devices に成果物が配置されます。

## ビルド手順 (macOS)
1. AppleのサイトよりXcodeコマンドラインツールをインストールします

2. Homebrewにより下記パッケージをインストールします
   ```sh
   brew install pkg-config cmake
   ```

3. プロジェクトをビルドします
   ```sh
   cmake -S . -B build
   cmake --build build
   ```

   build/list-devices に成果物が配置されます。

## ビルド手順 (Windows)
WSL2を使用して上記Ubuntuの手順をご利用ください。

## 実行

```
$ build/list-devices --apikey XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
{"count":1,"has_next":false,"list":[{"device_id":"ABCDEFGHIJKLMNOPQRST","model":{"description":"sample model"},"serial":"0123456789","setting":{"name":"sample device"},"status":{"video_streaming":true}}],"offset":0,"total":1}
```
