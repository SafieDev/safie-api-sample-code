import requests
from oauthlib.oauth2 import WebApplicationClient

#oauthクライアントの作成
SAFIE_API_CLIENT_ID = '5fdba7ddda8c'
oauth = WebApplicationClient(SAFIE_API_CLIENT_ID)

#アクセストークンの設定
oauth.access_token = '321aa06a-27a4-41c5-8895-785fd82fcf5f'
device_id = 'pJGayEpIDw9x4uUnHpq5'
url = 'https://openapi.safie.link/v1/devices/'+str(device_id)+'/image'
url, headers, body = oauth.add_token(url)

#画像の取得
r = requests.get(url, headers=headers)
print(r.status_code)

'''
画像のダウンロード
with open("test.jpeg", "wb") as fout:
        fout.write(r.content)
'''