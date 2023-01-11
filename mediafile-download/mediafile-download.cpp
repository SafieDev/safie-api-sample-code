/*
 * mediafile-download
 * Safie APIのAPIキー認証により録画をmp4ファイルとしてダウンロード
 *
 * Copyright (c) 2023 Safie Inc.
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

// 可変長バッファの構造体
typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} buffer;

size_t on_curl_write_buffer(char *ptr, size_t size, size_t nmemb,
                            void *userdata) {
  size_t realsize = size * nmemb;
  buffer *buf = (buffer *)userdata;
  if (buf->capacity - 1 < buf->size + realsize) {
    size_t newcapacity = buf->capacity * 2;
    CHECK_NULL(buf->data = (char *)realloc(buf->data, newcapacity));
    buf->capacity = newcapacity;
  }
  memcpy(buf->data + buf->size, ptr, realsize);
  buf->size += realsize;
  buf->data[buf->size] = '\0';
  return realsize;

error:
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

void print_help() {
  fprintf(
      stderr,
      "usage: media-download\n"
      "download recorded media by Safie API, using API key\n"
      "\n"
      "  -k, --apikey=APIKEY       API key, required\n"
      "  -d, --device-id=DEVICEID  device ID, required\n"
      "  -s, --start=DATETIME      start time of recorded media, required, "
      "in 'yyyy-mm-ddTHH:MM:SS' format\n"
      "  -e, --end=DATETIME        end time of recorded media, required, in "
      "'yyyy-mm-ddTHH:MM:SS' format\n"
      "  -h, --help                print this help\n");
}

/// @brief 「メディアファイル 作成要求」APIを実行します
/// @param api_key [IN] APIキー
/// @param device_id [IN] 対象デバイスID
/// @param start [IN] メディアの開始日時 (ローカル時間)
/// @param end [IN] メディアの終了日時 (ローカル時間)
/// @param request_id [OUT] リクエストID
/// @param verbosity [IN] `1` のときログ出力
/// @return 終了コード, `0` のとき正常終了
int post_request(const char *api_key, const char *device_id,
                 const struct tm start, const struct tm end, int *request_id,
                 int verbosity);

enum State {
  FAILED = 1,
  PROCESSING,
  AVAILABLE,
};

/// @brief 「メディアファイル 作成要求取得」APIを実行します
/// @param api_key [IN] APIキー
/// @param device_id [IN] 対象デバイスID
/// @param request_id [IN] リクエストID
/// @param state [OUT] メディアファイル作成状況
/// @param file_url [OUT] メディアファイル取得URL, `state == AVAILABLE`
/// のときのみ
/// @param verbosity [IN] `1` のときログ出力
/// @return 終了コード, `0` のとき正常終了
int get_request(const char *api_key, const char *device_id, int request_id,
                enum State *state, char **file_url, int verbosity);

/// @brief メディアファイルをダウンロードしファイルに保存します
/// @param api_key [IN] APIキー
/// @param url [IN] メディアファイルURL (「作成要求取得」APIで取得されたもの)
/// @param fp [IN] 出力ファイル
/// @param verbosity [IN] `1` のときログ出力
/// @return 終了コード, `0` のとき正常終了
int download_mediafile(const char *api_key, const char *url, FILE *fp,
                       int verbosity);

int main(int argc, char *argv[]) {
  /*
   * オプション引数の処理
   */
  const char *api_key = getenv("SAFIE_API_KEY");
  const char *device_id = NULL;
  struct tm start, end;
  memset(&start, 0, sizeof(struct tm));
  memset(&end, 0, sizeof(struct tm));
  const char *output_dir = ".";
  int verbosity = 0;

  int opt;
  static struct option long_options[] = {
      {"apikey", required_argument, NULL, 'k'},
      {"device-id", required_argument, NULL, 'd'},
      {"start", required_argument, NULL, 's'},
      {"end", required_argument, NULL, 'e'},
      {"output-dir", required_argument, NULL, 'o'},
      {"verbose", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, "k:d:s:e:o:vh", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'k':
      api_key = optarg;
      break;
    case 'd':
      device_id = optarg;
      break;
    case 's': {
      if (strptime(optarg, "%Y-%m-%dT%H:%M:%S", &start) == NULL) {
        fprintf(stderr, "error: invalid `--start`\n");
        print_help();
        exit(2);
      }
      break;
    }
    case 'e': {
      if (strptime(optarg, "%Y-%m-%dT%H:%M:%S", &end) == NULL) {
        fprintf(stderr, "error: invalid `--end`\n");
        print_help();
        exit(2);
      }
      break;
    }
    case 'o':
      output_dir = optarg;
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
  if (start.tm_year == 0) {
    fprintf(stderr, "error: missing start time\n");
    print_help();
    exit(2);
  }
  if (end.tm_year == 0) {
    fprintf(stderr, "error: missing end time\n");
    print_help();
    exit(2);
  }

  /*
   * メディアファイル作成の開始
   */
  FILE *fp = NULL;
  char *file_url = NULL;

  // メディアファイル作成要求
  fprintf(stderr, "requesting media file creation\n");
  int request_id;
  int rc = post_request(api_key, device_id, start, end, &request_id, verbosity);
  if (rc != 0) {
    goto error;
  }

  int i;
  for (i = 0; i < 10; i++) {
    fprintf(stderr, "awaiting 30 sec to complete...\n");
    sleep(30);

    // 30秒ごとに状況を取得
    enum State state;
    rc = get_request(api_key, device_id, request_id, &state, &file_url,
                     verbosity);
    if (rc != 0) {
      goto error;
    }
    if (state == FAILED) {
      // 処理失敗
      fprintf(stderr, "error: server reported media creation failed\n");
      goto error;
    }
    if (state == PROCESSING) {
      // 処理中
      continue;
    }

    char filename[256];
    int n = snprintf(filename, sizeof(filename), "%s/%d.mp4", output_dir,
                     request_id);
    if (n >= sizeof(filename)) {
      fprintf(stderr, "error: filename too long\n");
      goto error;
    }

    fp = fopen(filename, "w");
    if (fp == NULL) {
      fprintf(stderr, "error: failed to open file: %s\n", strerror(errno));
      goto error;
    }

    // ファイルをダウンロードする
    fprintf(stderr, "downloading media file to %s\n", filename);
    rc = download_mediafile(api_key, file_url, fp, verbosity);
    if (rc != 0) {
      goto error;
    }
    fclose(fp);
    fp = NULL;
    break;
  }
  if (i >= 10) {
    fprintf(stderr, "media creation did not complete in 300 sec\n");
    goto error;
  }

  free(file_url);
  return 0;

error:
  free(file_url);
  if (fp != NULL) {
    fclose(fp);
  }
  return 1;
}

int post_request(const char *api_key, const char *device_id,
                 const struct tm start, const struct tm end, int *request_id,
                 int verbosity) {
  struct curl_slist *headers = NULL;
  cJSON *req = NULL;
  buffer buf = {NULL, 0, 16384};
  CURL *curl = NULL;
  cJSON *res = NULL;
  CHECK_NULL(buf.data = (char *)malloc(16384));

  // リクエストURL
  char url[256];
  int n;
  n = snprintf(url, sizeof(url),
               "https://openapi.safie.link/v2/devices/%s/media_files/requests",
               device_id);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    goto error;
  }

  // Authorizationおよびほかのヘッダ
  char auth[64];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    goto error;
  }
  headers = curl_slist_append(headers, auth);
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // リクエストボディ
  CHECK_NULL(req = cJSON_CreateObject());
  {
    char dt[32];
    strftime(dt, sizeof(dt), "%Y-%m-%dT%H:%M:%S%z", &start);
    // strftimeの%zをRFC3339のtime-numoffsetに変更
    memmove(dt + 23, dt + 22, 3);
    dt[22] = ':';
    CHECK_NULL(cJSON_AddStringToObject(req, "start", dt));
  }
  {
    char dt[32];
    strftime(dt, sizeof(dt), "%Y-%m-%dT%H:%M:%S%z", &end);
    // strftimeの%zをRFC3339のtime-numoffsetに変更
    memmove(dt + 23, dt + 22, 3);
    dt[22] = ':';
    CHECK_NULL(cJSON_AddStringToObject(req, "end", dt));
  }

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_PrintUnformatted(req));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_curl_write_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
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

  // レスポンスの処理
  CHECK_NULL(res = cJSON_Parse(buf.data));
  cJSON *el;
  el = cJSON_GetObjectItemCaseSensitive(res, "request_id");
  if (!cJSON_IsNumber(el)) {
    fprintf(stderr, "error: invalid response\n");
    goto error;
  }
  *request_id = el->valueint;

  cJSON_Delete(res);
  free(buf.data);
  curl_easy_cleanup(curl);
  cJSON_Delete(req);
  curl_slist_free_all(headers);
  return 0;

error:
  cJSON_Delete(res);
  free(buf.data);
  curl_easy_cleanup(curl);
  cJSON_Delete(req);
  curl_slist_free_all(headers);
  return 1;
}

int get_request(const char *api_key, const char *device_id, int request_id,
                enum State *state, char **file_url, int verbosity) {
  struct curl_slist *headers = NULL;
  CURL *curl = NULL;
  buffer buf = {NULL, 0, 16384};
  cJSON *res = NULL;
  CHECK_NULL(buf.data = (char *)malloc(16384));

  char url[256];
  int n;
  n = snprintf(
      url, sizeof(url),
      "https://openapi.safie.link/v2/devices/%s/media_files/requests/%d",
      device_id, request_id);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    goto error;
  }

  char auth[64];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    goto error;
  }

  headers = curl_slist_append(headers, auth);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_curl_write_buffer);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
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

  CHECK_NULL(res = cJSON_Parse(buf.data));

  cJSON *el_state;
  el_state = cJSON_GetObjectItemCaseSensitive(res, "state");
  if (!cJSON_IsString(el_state)) {
    fprintf(stderr, "error: invalid response\n");
    goto error;
  }
  if (strcmp(el_state->valuestring, "FAILED") == 0) {
    *state = FAILED;
  } else if (strcmp(el_state->valuestring, "PROCESSING") == 0) {
    *state = PROCESSING;
  } else if (strcmp(el_state->valuestring, "AVAILABLE") == 0) {
    *state = AVAILABLE;
  } else {
    fprintf(stderr, "error: invalid response\n");
    goto error;
  }

  cJSON *el_url;
  el_url = cJSON_GetObjectItemCaseSensitive(res, "url");
  if (cJSON_IsString(el_url)) {
    *file_url = strdup(el_url->valuestring);
  } else if (*state == AVAILABLE) {
    fprintf(stderr, "error: invalid response\n");
    goto error;
  }

  cJSON_Delete(res);
  free(buf.data);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 0;

error:
  cJSON_Delete(res);
  free(buf.data);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 1;
}

int download_mediafile(const char *api_key, const char *url, FILE *fp,
                       int verbosity) {
  struct curl_slist *headers = NULL;
  CURL *curl = NULL;

  char auth[64];
  int n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    goto error;
  }

  headers = curl_slist_append(headers, auth);

  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  // libcurlのデフォルトコールバックを使用
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, (verbosity) ? 1 : 0);

  CURLcode ret;
  ret = curl_easy_perform(curl);
  if (ret != CURLE_OK) {
    fprintf(stderr, "error: curl failed: %d: %s\n", ret,
            curl_easy_strerror(ret));
    goto error;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 0;

error:
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 1;
}
