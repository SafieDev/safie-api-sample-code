/*
 * get-image-set-flag
 * Safie APIよりカメラ画像を取得し画像解析結果に応じてイベントを登録する
 *
 * Copyright (c) 2023 Safie Inc.
 */
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <cjson/cJSON.h>
#include <curl/curl.h>
}

/// @brief ポインタを返す `expr` を評価し値がNULLのときerrorラベルにjumpします
/// @param expr ポインタを返す式
#define CHECK_NULL(expr)                                                       \
  {                                                                            \
    if ((expr) == NULL) {                                                      \
      fprintf(stderr, "error: \"%s\": NULL at %s(%d)\n", #expr, __FILE__,      \
              __LINE__);                                                       \
      goto error;                                                              \
    }                                                                          \
  }

typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} buffer;

/// @brief Safie APIによりカメラ画像を取得する
/// @param api_key [IN] Safie APIのAPIキー
/// @param device_id [IN] 対象カメラのデバイスID
/// @param buf [IN/OUT] 取得された画像 (JPEG) を保存するバッファ
/// @param verbosity [IN] 値が0以上のとき詳細なログメッセージを出力する
/// @return 終了コード、0以外のときエラー
int get_device_image(const char *api_key, const char *device_id, buffer *buf,
                     int verbosity);

/// @brief 画像の分析を行う (現在の実装はプレースホルダ)
/// @param buf [IN] 取得された画像 (JPEG) のバッファ
/// @param score [OUT] 報告すべき事象が存在するかのしきい値 [0, 1]
/// @return 終了コード、0以外のときエラー
int analyze(buffer *buf, double *score);

/// @brief Safie APIによりイベント (SafieViewerのVODタイムライン上のピン) を登録
/// @param api_key [IN] Safie APIのAPIキー
/// @param device_id [IN] 対象カメラのデバイスID
/// @param definition_id [IN] イベント定義ID
/// @param verbosity [IN] 値が0以上のとき詳細なログメッセージを出力する
/// @return 終了コード、0以外のときエラー
int post_event(const char *api_key, const char *device_id,
               const char *definition_id, int verbosity);

void print_help() {
  fprintf(
      stderr,
      "usage: get-image-set-flag [OPTIONS]...\n"
      "get camera image by Safie API and register camera event according to\n"
      "image analysis result\n"
      "\n"
      "  -k, --apikey=APIKEY       API key, required\n"
      "  -d, --device-id=DEVICEID  device ID, required\n"
      "  -e, --definition-id=ID    event definition ID, required\n"
      "  -v, --verbose             enable verbose logging\n"
      "  -h, --help                print this help\n");
}

int main(int argc, char *argv[]) {
  /*
   * オプション引数の処理
   */
  const char *api_key = getenv("SAFIE_API_KEY");
  const char *device_id = NULL;
  const char *definition_id = NULL;
  int verbosity = 0;

  int opt;
  static struct option long_options[] = {
      {"apikey", required_argument, NULL, 'k'},
      {"device-id", required_argument, NULL, 'd'},
      {"definition-id", required_argument, NULL, 'e'},
      {"verbose", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, "k:d:e:vh", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'k':
      api_key = optarg;
      break;
    case 'd':
      device_id = optarg;
      break;
    case 'e':
      definition_id = optarg;
      break;
    case 'v':
      verbosity++;
      break;
    case 'h':
      print_help();
      exit(0);
    default:
      print_help();
      exit(2);
    }
  }
  if (optind != argc) {
    print_help();
    exit(2);
  }
  if (api_key == NULL) {
    fprintf(stderr, "error: missing API key\n");
    print_help();
    exit(2);
  }
  if (device_id == NULL) {
    fprintf(stderr, "error: missing device ID\n");
    print_help();
    exit(2);
  }
  if (definition_id == NULL) {
    fprintf(stderr, "error: missing event definition ID\n");
    print_help();
    exit(2);
  }

  /*
   * メインループ
   */
  int rc;
  buffer buf = {NULL, 0, 16384};
  CHECK_NULL(buf.data = (char *)malloc(16384));

  int dead_time;
  dead_time = 0;
  while (1) {
    // カメラ画像を取得
    buf.size = 0;
    rc = get_device_image(api_key, device_id, &buf, verbosity);
    if (rc != 0) {
      goto error;
    }

    // 画像解析処理を実施しスコアを計算
    // (画像分類や検出の信頼度を想定)
    double score = 0.0;
    rc = analyze(&buf, &score);
    if (rc != 0) {
      goto error;
    }

    int found = (score > 0.95) ? 1 : 0;
    if (found && dead_time <= 0) {
      // スコアがしきい値を超えかつ過去15秒間で検出がないときイベント登録
      fprintf(stderr, "event found, registering: score=%f\n", score);
      rc = post_event(api_key, device_id, definition_id, verbosity);
      if (rc != 0) {
        goto error;
      }
    } else if (found) {
      // スコアがしきい値を超え過去15秒で検出があるとき検出を無視する
      fprintf(stderr, "event found but ignored: score=%f\n", score);
    } else {
      // スコアがしきい値以下
      fprintf(stderr, "event not found: score=%f\n", score);
    }

    // 不感時間の更新
    if (found) {
      dead_time = 3;
    } else {
      dead_time--;
    }

    // 1tickごと5秒ウェイト
    sleep(5);
  }

  free(buf.data);
  return 0;

error:
  free(buf.data);
  return 1;
}

size_t on_curl_write_buffer(char *ptr, size_t size, size_t nmemb,
                            void *userdata) {
  size_t realsize = size * nmemb;
  buffer *buf = (buffer *)userdata;
  if (buf->capacity < buf->size + realsize) {
    size_t newcapacity = buf->capacity * 2;
    CHECK_NULL(buf->data = (char *)realloc(buf->data, newcapacity));
    buf->capacity = newcapacity;
  }
  memcpy(buf->data + buf->size, ptr, realsize);
  buf->size += realsize;
  return realsize;

error:
  return 0;
}

int get_device_image(const char *api_key, const char *device_id, buffer *buf,
                     int verbosity) {
  struct curl_slist *headers = NULL;
  CURL *curl = NULL;

  // リクエストURL
  char url[256];
  int n;
  n = snprintf(url, sizeof(url),
               "https://openapi.safie.link/v2/devices/%s/image", device_id);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    goto error;
  }

  // Safie-API-Keyおよびその他ヘッダ
  char auth[64];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    goto error;
  }
  headers = curl_slist_append(headers, auth);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // HTTP要求
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_curl_write_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, (verbosity) ? 1 : 0);

  CURLcode ret;
  ret = curl_easy_perform(curl);
  if (ret != CURLE_OK) {
    fprintf(stderr, "error: curl failed: %d: %s\n", ret,
            curl_easy_strerror(ret));
    goto error;
  }

  // 後処理
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 0;

error:
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 1;
}

int analyze(buffer *buf, double *score) {
  // サンプルでは60秒周期のサインカーブを出力
  *score = sin(time(NULL) / 60.0 * M_PI * 2.0);
  return 0;
}

size_t on_curl_debug(CURL *handle, curl_infotype type, char *data, size_t size,
                     void *userptr) {
  switch (type) {
  case CURLINFO_DATA_IN:
    fprintf(stderr, "IN: ");
    fwrite(data, size, 1, stderr);
    fprintf(stderr, "\n\n");
    break;
  default:
    break;
  }
  return 0;
}

int post_event(const char *api_key, const char *device_id,
               const char *definition_id, int verbosity) {
  struct curl_slist *headers = NULL;
  cJSON *req = NULL;
  CURL *curl = NULL;

  // リクエストURL
  char url[256];
  int n;
  n = snprintf(url, sizeof(url),
               "https://openapi.safie.link/v2/devices/%s/events", device_id);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    goto error;
  }

  // Safie-API-Keyおよびその他ヘッダ
  char auth[64];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    goto error;
  }
  headers = curl_slist_append(headers, auth);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // body
  CHECK_NULL(req = cJSON_CreateObject());
  CHECK_NULL(cJSON_AddStringToObject(req, "definition_id", definition_id));

  // HTTP要求
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_PrintUnformatted(req));
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, (verbosity) ? 1 : 0);
  curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, on_curl_debug);

  CURLcode ret;
  ret = curl_easy_perform(curl);
  if (ret != CURLE_OK) {
    fprintf(stderr, "error: curl failed: %d: %s\n", ret,
            curl_easy_strerror(ret));
    goto error;
  }

  // 後処理
  curl_easy_cleanup(curl);
  cJSON_Delete(req);
  curl_slist_free_all(headers);
  return 0;

error:
  curl_easy_cleanup(curl);
  cJSON_Delete(req);
  curl_slist_free_all(headers);
  return 1;
}
