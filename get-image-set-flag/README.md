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
analyze result: score=0.8134851425223426, is_event_detected=False
downloaded latest device image (29200 bytes)
analyze result: score=0.39079039713152247, is_event_detected=False
downloaded latest device image (29331 bytes)
analyze result: score=-0.14834041117833938, is_event_detected=False
downloaded latest device image (29300 bytes)
analyze result: score=-0.6400877090181488, is_event_detected=False
downloaded latest device image (29063 bytes)
analyze result: score=-0.9458060430010351, is_event_detected=False
downloaded latest device image (29180 bytes)
analyze result: score=-0.9771801874861765, is_event_detected=False
downloaded latest device image (28951 bytes)
analyze result: score=-0.725403944518527, is_event_detected=False
downloaded latest device image (29266 bytes)
analyze result: score=-0.2618391854190125, is_event_detected=False
downloaded latest device image (29021 bytes)
analyze result: score=0.27853239790292766, is_event_detected=False
downloaded latest device image (29045 bytes)
analyze result: score=0.7357504499566003, is_event_detected=False
downloaded latest device image (28871 bytes)
analyze result: score=0.9806253885342279, is_event_detected=True
event is detected! so posted event
...
```

