import multiprocessing
import secrets
import urllib.parse
import webbrowser

import click
import uvicorn
from fastapi import FastAPI
from fastapi.responses import PlainTextResponse
import requests


SAFIE_API_BASE_URL = "https://openapi.safie.link"


@click.group()
def cli():
    pass


@cli.command()
@click.option(
    "--client-id", type=str, required=True, envvar="CLIENT_ID", help="OAuth2 クライアントID"
)
@click.option(
    "--client-secret",
    type=str,
    required=True,
    envvar="CLIENT_SECRET",
    help="OAuth2 クライアントシークレット",
)
def auth(client_id: str, client_secret: str):
    """
    OAuth2 Authorization Code Flowに基づき、ユーザのアクセストークンを取得します

    \b
    - 認可用ユーザ認証用ページをブラウザで表示
    - 認証後、リダイレクトURL (http://localhost:8000) に認可コードが付与されたリクエストが行われる
    - 認可コードを取得し、それを利用してアクセストークンを取得する
    """
    queue = multiprocessing.Queue()
    proc = multiprocessing.Process(target=run_app, args=(queue,), daemon=True)
    proc.start()

    state = secrets.token_hex(16)
    url = f"{SAFIE_API_BASE_URL}/v2/auth/authorize?" + urllib.parse.urlencode(
        [
            ("client_id", client_id),
            ("response_type", "code"),
            ("scope", "safie-api"),
            ("redirect_uri", "http://localhost:8000/callback"),
            ("state", state),
        ],
        doseq=True,
    )
    webbrowser.open(url)

    data = queue.get(block=True)
    proc.terminate()

    assert state == data["state"]

    res = requests.post(
        f"{SAFIE_API_BASE_URL}/v2/auth/token",
        data={
            "client_id": client_id,
            "client_secret": client_secret,
            "grant_type": "authorization_code",
            "redirect_uri": "http://localhost:8000/callback",
            "code": data["code"],
        },
    )
    res.raise_for_status()
    print(res.text)


@cli.command()
@click.option(
    "--client-id", type=str, required=True, envvar="CLIENT_ID", help="OAuth2 クライアントID"
)
@click.option(
    "--client-secret",
    type=str,
    required=True,
    envvar="CLIENT_SECRET",
    help="OAuth2 クライアントシークレット",
)
@click.option("--refresh-token", type=str, required=True, help="リフレッシュトークン")
def refresh(client_id: str, client_secret: str, refresh_token: str):
    """
    アクセストークンのリフレッシュを行います
    """
    res = requests.post(
        f"{SAFIE_API_BASE_URL}/v2/auth/refresh-token",
        data={
            "client_id": client_id,
            "client_secret": client_secret,
            "grant_type": "refresh_token",
            "refresh_token": refresh_token,
            "scope": "safie-api",
        },
    )
    res.raise_for_status()
    print(res.text)


def run_app(queue: multiprocessing.Queue):
    app = FastAPI()

    @app.get("/callback", response_class=PlainTextResponse)
    def oauth2_callback(code: str, state: str):
        queue.put({"code": code, "state": state})
        return "Got oauth2 authorization code !"

    uvicorn.run(app)


if __name__ == "__main__":
    cli()
