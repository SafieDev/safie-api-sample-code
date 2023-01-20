import json
import os
from datetime import timedelta, timezone, datetime
from time import sleep

import click
import requests as requests


SAFIE_API_BASE_URL = "https://openapi.safie.link"


@click.command()
@click.option("--apikey", type=str, required=True, envvar="SAFIE_API_KEY", help="APIキー")
@click.option("--device-id", type=str, required=True, help="デバイスID")
@click.option(
    "--start",
    type=click.DateTime(formats=["%Y-%m-%dT%H:%M:%S"]),
    required=True,
    help="作成するメディアの開始日時(ex. 2022-11-12T16:41:00) JSTの日時を指定してください 作成可能なファイルの録画時間は1分から10分です",
)
@click.option(
    "--end",
    type=click.DateTime(formats=["%Y-%m-%dT%H:%M:%S"]),
    required=True,
    help="作成するメディアの終了日時(ex. 2022-11-12T16:42:00) JSTの日時を指定してください 作成可能なファイルの録画時間は1分から10分です",
)
def download_media_file(apikey: str, device_id: str, start: datetime, end: datetime):
    """
    APIキー認証を利用してメディアファイルのダウンロードを行います

    \b
    - メディアファイル作成要求
    - メディアファイル作成状況の確認(30秒間隔で最大10回)
    - メディアファイルダウンロード
    """
    JST = timezone(timedelta(hours=9), "JST")
    start = start.replace(tzinfo=JST)
    end = end.replace(tzinfo=JST)
    payload = {
        "start": start.isoformat(),
        "end": end.isoformat(),
    }

    # メディアファイル作成要求APIの呼び出し
    response = requests.post(
        f"{SAFIE_API_BASE_URL}/v2/devices/{device_id}/media_files/requests",
        headers={"Safie-API-Key": apikey},
        data=json.dumps(payload),
    )
    response.raise_for_status()
    print("メディアファイル作成要求APIレスポンス")
    print(json.dumps(response.json(), indent=4, sort_keys=True))

    # メディアファイルリクエストIDをメディアファイル作成要求APIのレスポンスから取得
    request_id = response.json()["request_id"]

    # メディアファイル作成要求取得APIの呼び出し(メディアファイル作成状況の確認)
    # メディアファイルの作成が完了するか失敗するまで、30秒間隔で最大10回呼び出す
    check_count = 10
    for i in range(check_count):
        sleep(30)
        response = requests.get(
            f"{SAFIE_API_BASE_URL}/v2/devices/{device_id}/media_files/requests/{request_id}",
            headers={"Safie-API-Key": apikey},
        )
        response.raise_for_status()
        print("メディアファイル作成要求取得APIレスポンス")
        print(json.dumps(response.json(), indent=4, sort_keys=True))

        # メディアファイルの作成状況をメディアファイル作成要求取得APIのレスポンスから取得
        state = response.json()["state"]

        # メディアファイルの作成状況が作成失敗の場合は処理を終了する
        if state == "FAILED":
            raise Exception("メディアファイルの作成に失敗しました")

        # メディアファイルの作成状況が作成中の場合は、メディアファイル作成要求取得APIを繰り返し呼び出す
        elif state == "PROCESSING":
            # 最大試行回数に達した場合は処理を終了する
            if i == check_count - 1:
                raise Exception("規定の時間内にメディアファイル作成が終了しませんでした")

            continue

        # メディアファイルの作成状況が作成完了の場合は、メディアファイルのダウンロードを行う
        elif state == "AVAILABLE":
            # メディアファイルのダウンロードURLをメディアファイル作成要求取得APIのレスポンスから取得
            url = response.json()["url"]

            # メディアファイルダウンロードAPIの呼び出し
            response = requests.get(url, headers={"Safie-API-Key": apikey}, stream=True)
            response.raise_for_status()

            # ダウンロードしたファイルを保存する
            file_name = os.path.dirname(__file__) + f"/{request_id}.mp4"
            with open(file_name, "wb") as fw:
                for chunk in response.iter_content(chunk_size=100 * 1024):
                    fw.write(chunk)
            print(f"メディアファイルをダウンロードしました。 ダウンロードファイル: {file_name}")
            break


if __name__ == "__main__":
    download_media_file()
