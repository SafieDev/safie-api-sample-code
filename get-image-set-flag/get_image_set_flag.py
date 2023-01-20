import time
import requests
import math
from datetime import datetime
import click


SAFIE_API_BASE_URL = "https://openapi.safie.link"


def get_device_image(apikey: str, device_id: str) -> bytes:
    """
    デバイス画像取得APIを利用し、デバイスから最新の画像を取得します
    """
    res = requests.get(
        url=f"{SAFIE_API_BASE_URL}/v2/devices/{device_id}/image",
        headers={"Safie-API-Key": apikey},
    )
    res.raise_for_status()
    return res.content


def analyze(image: bytes, dead_time: int) -> (bool, int):
    """
    画像解析を行いイベントが発生したかどうかを判定します
    サンプルでは実際の解析は行わず、60秒周期のsinの値から判定結果を返します
    """
    score = math.sin(datetime.now().timestamp() / 60 * math.pi * 2)
    is_found = score > 0.95
    print(f"analyze result: score={score}, is_found={is_found}, dead_time={dead_time}")
    if is_found and dead_time <= 0:  # スコアがしきい値を超えかつ過去15秒間で検出がないときイベント登録
        dead_time = 3
        return True, dead_time
    elif is_found:  # スコアがしきい値を超え過去15秒で検出があるとき検出を無視する
        print("event found but ignored")
        dead_time -= 1
        return False, dead_time
    else:
        print("event not found")
        dead_time -= 1
        return False, dead_time


def post_event(apikey: str, device_id: str, definition_id: str):
    """
    イベント登録APIを利用し、デバイスに対してイベント登録を行います
    """
    res = requests.post(
        url=f"{SAFIE_API_BASE_URL}/v2/devices/{device_id}/events",
        headers={"Safie-API-Key": apikey},
        json={"definition_id": definition_id},
    )
    res.raise_for_status()


@click.command()
@click.option("--apikey", type=str, required=True, envvar="SAFIE_API_KEY", help="APIキー")
@click.option("--device-id", type=str, required=True, help="利用するカメラのデバイスID")
@click.option("--definition-id", type=str, required=True, help="利用するイベント定義ID")
def main(apikey: str, device_id: str, definition_id: str):
    """
    5秒おきに以下のタスクを実行します

    \b
    - デバイスから最新画像取得
    - 取得した画像からイベントが発生したかを判定
    - イベント発生を検知した場合、デバイスに対してイベントを登録
    """
    dead_time = 0
    while True:
        image = get_device_image(apikey=apikey, device_id=device_id)
        print(f"downloaded latest device image ({len(image)} bytes)")
        is_event_detected, dead_time = analyze(image, dead_time)
        if is_event_detected:
            post_event(apikey=apikey, device_id=device_id, definition_id=definition_id)
            print("event found! so posted event")
        time.sleep(5)


if __name__ == "__main__":
    main()
