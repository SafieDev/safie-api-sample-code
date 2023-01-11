ここでは以下の実行するためのサンプルコードを掲載しています。
- OAuth2 Authorization Code Flowに基づくユーザのアクセストークン取得およびアクセストークンのリフレッシュ
- 取得したアクセストークンを利用したデバイス一覧の取得

# Python 実装

## 必須要件
- Python 3.10
- poetry

## 環境構築

Poetryを用いて実行環境を構築します。

```shell
$ poetry install
```

Safie Developersで作成したOAuth2.0認証のアプリケーションにおいて、クライアントIDとクライアントシークレットを発行し、そのクライアントのリダイレクトURLに `http://localhost:8000/callback` を設定してください。

## 実行

### アクセストークン取得、リフレッシュ

以下のコマンドでアクセストークンを取得できます。

Safie Developers から取得したクライアントID/クライアントシークレットをオプション `--client-id`/`--client-secret` もしくは環境変数 `CLIENT_ID`/`CLIENT_SECRET` に設定して実行します。

実行するとブラウザでユーザ認証用ページが表示されるのでメールアドレスとパスワードを入力してください。
認証に成功するとアクセストークンの取得が行われ、コンソールにアクセストークンとリフレッシュトークンが出力されます。

```shell
$ poetry run python authorization_oauth2.py auth --client-id XXXXXXXXXX --client-secret XXXXXXXXXXXXXXXXXXXXXXXXXXX
INFO:     Started server process [15677]
INFO:     Waiting for application startup.
INFO:     Application startup complete.
INFO:     Uvicorn running on http://127.0.0.1:8000 (Press CTRL+C to quit)
INFO:     127.0.0.1:53722 - "GET /callback?code=xxxxxxxxxxxxxxxxxxxxx HTTP/1.1" 200 OK
INFO:     Shutting down
INFO:     Waiting for application shutdown.
INFO:     Application shutdown complete.
INFO:     Finished server process [15677]
{"access_token":"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX","token_type":"Bearer","expires_in":604800,"refresh_token":"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"}
```

以下のコマンドでアクセストークンのリフレッシュを行うことができます。
Safie Developers から取得したクライアントID/クライアントシークレットに加えて、上記コマンドで取得したリフレッシュトークンを指定します。

```shell
$ poetry run python authorization_oauth2.py refresh --client-id XXXXXXXXXX --client-secret XXXXXXXXXXXXXXXXXXXXXXXXXXX --refresh-token XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
{"access_token":"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX","token_type":"Bearer","expires_in":604800,"refresh_token":"XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"}
```

### デバイス一覧の取得

以下のコマンドでデバイス一覧をJSON形式で取得することができます。
上記コマンドで取得したアクセストークンをオプション `--access-token` もしくは環境変数 `SAFIE_ACCESS_TOKEN` に設定して実行します。

```shell
$ poetry run python list_devices.py --access-token XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
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

  OAuth2認証を利用してデバイス一覧を取得します

Options:
  --access-token TEXT     アクセストークン  [required]
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

### アクセストークン取得、リフレッシュ
アクセストークンの取得およびリフレッシュはPython実装をご利用ください。

### デバイス一覧の取得

```
$ build/list-devices --access-token XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
{"count":1,"has_next":false,"list":[{"device_id":"ABCDEFGHIJKLMNOPQRST","model":{"description":"sample model"},"serial":"0123456789","setting":{"name":"sample device"},"status":{"video_streaming":true}}],"offset":0,"total":1}
```
