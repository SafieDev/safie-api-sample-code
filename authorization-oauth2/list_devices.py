import json
import click
import requests
from typing import Optional


SAFIE_API_BASE_URL = "https://openapi.safie.link"


@click.command()
@click.option(
    "--access-token",
    type=str,
    required=True,
    envvar="SAFIE_ACCESS_TOKEN",
    help="アクセストークン",
)
@click.option(
    "--offset",
    type=click.IntRange(0, 10000),
    default=0,
    help="返却するリストのオフセット (デフォルト値: 0)",
)
@click.option(
    "--limit", type=click.IntRange(1, 100), default=20, help="返却するリストの最大件数 (デフォルト値: 20"
)
@click.option(
    "--item-id",
    type=int,
    default=None,
    help="デバイスに付与されているオプションによる絞り込み (具体的な値は https://developers.safie.link/terms-of-api をご参照ください",
)
@click.option(
    "--all", is_flag=True, help="全てのデバイス一覧を取得する (このオプションを指定した場合、他のオプション指定は無視されます)"
)
def list_devices(
    access_token: str, offset: int, limit: int, item_id: Optional[int], all: bool
) -> None:
    """
    OAuth2認証を利用してデバイス一覧を取得します
    """
    if all:
        offset, limit, item_id = 0, 100, None

    has_next, current_offset = True, offset
    while has_next:
        params = {"offset": current_offset, "limit": limit}
        if item_id is not None:
            params["item_id"] = item_id

        res = requests.get(
            url=f"{SAFIE_API_BASE_URL}/v2/devices",
            headers={"Authorization": f"Bearer {access_token}"},
            params=params,
        )
        res.raise_for_status()
        json_res = res.json()
        print(json.dumps(json_res, indent=4, sort_keys=True))
        has_next = json_res["has_next"] if all else False
        current_offset += json_res["count"]


if __name__ == "__main__":
    list_devices()
