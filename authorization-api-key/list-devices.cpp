/*
 * list-devices
 * Safie APIのAPIキー認証によりデバイス一覧を取得
 *
 * Copyright (c) 2023 Safie Inc.
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <curl/curl.h>
}

static size_t on_curl_write(char *ptr, size_t size, size_t nmemb,
                            void *userdata) {
  // APIのレスポンスボディをstdoutに出力
  fwrite(ptr, size, nmemb, stdout);
  return size * nmemb;
}

void print_help() {
  fprintf(stderr,
          "usage: list-devices\n"
          "list devices by Safie API, using API key\n"
          "\n"
          "  -k, --apikey=APIKEY       API key, required\n"
          "  -o, --offset=0            items offset, [0, )\n"
          "  -l, --limit=20            items limit, [0, 100]\n"
          "  -i, --item-id=ITEMID      filter devices by attached option plan\n"
          "  -h, --help                print this help\n");
}

int main(int argc, char *argv[]) {
  /*
   * オプション引数の処理
   */
  const char *api_key = getenv("SAFIE_API_KEY");
  int offset = 0, limit = 20, item_id = -1;

  int opt;
  static struct option long_options[] = {
      {"apikey", required_argument, NULL, 'k'},
      {"offset", required_argument, NULL, 'o'},
      {"limit", required_argument, NULL, 'l'},
      {"item-id", required_argument, NULL, 'i'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, "k:o:l:i:h", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'k':
      api_key = optarg;
      break;
    case 'o':
      errno = 0;
      offset = strtol(optarg, NULL, 10);
      if (errno != 0 || offset < 0) {
        fprintf(stderr, "error: invalid offset\n");
        print_help();
        exit(2);
      }
      break;
    case 'l':
      errno = 0;
      limit = strtol(optarg, NULL, 10);
      if (errno != 0 || limit < 0 || 100 < limit) {
        fprintf(stderr, "error: invalid limit\n");
        print_help();
        exit(2);
      }
      break;
    case 'i':
      errno = 0;
      item_id = strtol(optarg, NULL, 10);
      if (errno != 0 || item_id < 0) {
        fprintf(stderr, "error: invalid item-id\n");
        print_help();
        exit(2);
      }
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

  /*
   * デバイス一覧APIへのアクセス
   */
  char url[512];
  int n = snprintf(url, sizeof(url),
                   "https://openapi.safie.link/v2/devices"
                   "?offset=%d"
                   "&ilmit=%d",
                   offset, limit);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    exit(1);
  }
  if (0 <= item_id) {
    size_t b = strlen(url);
    n = snprintf(url + b, sizeof(url) - b, "?item_id=%d", item_id);
    if (b + n >= sizeof(url)) {
      fprintf(stderr, "error: url too long\n");
      exit(1);
    }
  }

  char auth[64];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s", api_key);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: api-key too long\n");
    exit(1);
  }

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, auth);

  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_curl_write);

  CURLcode ret = curl_easy_perform(curl);
  fprintf(stdout, "\n");
  if (ret != CURLE_OK) {
    fprintf(stderr, "error: curl failed: %d: %s\n", ret,
            curl_easy_strerror(ret));
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    exit(1);
  }

  long response_code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
  if (response_code != 200) {
    fprintf(stderr, "error: request non-successful: %ld\n", response_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    exit(1);
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);
  return 0;
}
