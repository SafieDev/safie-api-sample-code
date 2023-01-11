/*
 * streaming-download
 * Safie Streaming APIから映像をダウンロードしmp4ファイルに分割して保存する。
 *
 * Copyright (c) 2023 Safie Inc.
 */
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <libavformat/avformat.h>
}

/// @brief `AVError` を返す `expr` を評価し値が0以下のときラベル `end`
/// にジャンプします
/// @param expr `AVError` を返す式
#define CHECK_AVERROR(expr)                                                    \
  {                                                                            \
    int ret = (expr);                                                          \
    if (ret < 0) {                                                             \
      char buf[64];                                                            \
      av_strerror(ret, buf, sizeof(buf));                                      \
      fprintf(stderr, "error: \"%s\": %s at %s(%d)\n", #expr, buf, __FILE__,   \
              __LINE__);                                                       \
      goto error;                                                              \
    }                                                                          \
  }

/// @brief ポインタを返す `expr` を評価し値がNULLのときプログラムを終了します
/// @param expr ポインタを返す式
#define CHECK_NULL(expr)                                                       \
  {                                                                            \
    if ((expr) == NULL) {                                                      \
      fprintf(stderr, "error: \"%s\": NULL at %s(%d)\n", #expr, __FILE__,      \
              __LINE__);                                                       \
      goto error;                                                              \
    }                                                                          \
  }

// プログラムの説明文を表示する
void print_help() {
  fprintf(stderr,
          "usage: streaming-download [OPTION]...\n"
          "obtain camera image from Safie Streaming API and save to mp4 file.\n"
          "\n"
          "  -k, --apikey=APIKEY     API key, required\n"
          "  -d, --device-id=DEVICEID camera ID to obtain image from\n"
          "  -o, --output-dir=.       output directory\n"
          "  -d, --split-duration=60  split duration [sec] of output MP4 file\n"
          "  -v, --verbose            enable verbose logging from FFmpeg\n"
          "  -h, --help               shows this help\n");
}

// Ctrl+Cによりプログラムの終了を通知する
static volatile sig_atomic_t stopping = 0;
void sighandler(int signum) {
  stopping = 1;
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);
}

// メイン関数
int main(int argc, char *argv[]) {
  /*
   * 引数の処理
   */
  char *device_id = NULL;
  char *apikey = getenv("SAFIE_API_KEY");
  double duration = 60.0;
  const char *output_dir = ".";
  int verbosity = 0;

  int opt;
  static struct option long_options[] = {
      {"apikey", required_argument, NULL, 'k'},
      {"device-id", required_argument, NULL, 'd'},
      {"output-dir", required_argument, NULL, 'o'},
      {"split-duration", required_argument, NULL, 's'},
      {"verbose", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
  };

  while ((opt = getopt_long(argc, argv, "k:d:o:s:vh", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'k':
      apikey = optarg;
      break;
    case 'd':
      device_id = optarg;
      break;
    case 'o':
      output_dir = optarg;
      break;
    case 's':
      duration = strtod(optarg, NULL);
      if (duration <= 0.0) {
        print_help();
        exit(2);
      }
      break;
    case 'v':
      verbosity++;
      break;
    case 'h':
      print_help();
      exit(0);
      break;
    default:
      print_help();
      exit(2);
      break;
    }
  }

  if (optind != argc) {
    print_help();
    exit(2);
  }
  if (apikey == NULL) {
    fprintf(stderr, "error: missing --api-key\n");
    print_help();
    exit(2);
  }
  if (device_id == NULL) {
    fprintf(stderr, "error: missing --device-id\n");
    print_help();
    exit(2);
  }

  av_log_set_level(AV_LOG_WARNING + (8 * verbosity));

  /*
   * HLS接続
   */
  AVDictionary *dict = NULL;
  AVFormatContext *ic = NULL;
  AVPacket *pkt = av_packet_alloc();
  if (pkt == NULL) {
    fprintf(stderr, "error: \"av_packet_alloc()\": NULL at %s(%d)\n", __FILE__,
            __LINE__);
    exit(1);
  }
  AVFormatContext *oc = NULL;

  char url[256];
  int n =
      snprintf(url, sizeof(url),
               "https://openapi.safie.link/v2/devices/%s/live/playlist.m3u8",
               device_id);
  if (n >= sizeof(url)) {
    fprintf(stderr, "error: url too long\n");
    goto error;
  }

  char auth[256];
  n = snprintf(auth, sizeof(auth), "Safie-API-Key: %s\r\n", apikey);
  if (n >= sizeof(auth)) {
    fprintf(stderr, "error: auth header too long\n");
    goto error;
  }
  CHECK_AVERROR(av_dict_set(&dict, "headers", auth, 0));
  // リトライ回数を制限する
  CHECK_AVERROR(av_dict_set(&dict, "max_reload", "2", 0));
  CHECK_AVERROR(av_dict_set(&dict, "rw_timeout", "8000000", 0));

  CHECK_AVERROR(avformat_open_input(&ic, url, NULL, &dict));
  CHECK_AVERROR(avformat_find_stream_info(ic, NULL));
  int video_stream_index;
  CHECK_AVERROR(video_stream_index = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                                         -1, -1, NULL, 0));

  if (verbosity >= 1) {
    // 入力ストリームの情報表示
    av_dump_format(ic, 0, url, 0);
  }

  /*
   * ストリーム処理
   */
  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
  fprintf(stderr, "press Ctrl+C to stop\n");

  // 最初の1パケットを読む
  CHECK_AVERROR(av_read_frame(ic, pkt));
  while (!stopping) {
    // ファイル出力の最初のパケットのタイミングを計算
    int64_t pts_offset = pkt->pts;
    AVRational tb = ic->streams[pkt->stream_index]->time_base;
    int64_t end_pts = pkt->pts + (int64_t)(duration * tb.den / tb.num);

    /*
     * ファイル出力の設定
     */
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timestr[32];
    strftime(timestr, sizeof(timestr), "%F %H_%M_%S", lt);
    char filename[256];
    n = snprintf(filename, sizeof(filename), "%s/%s.mp4", output_dir, timestr);
    if (n >= sizeof(filename)) {
      fprintf(stderr, "error: filename too long\n");
      goto error;
    }

    fprintf(stderr, "writing file \"%s\"...\n", filename);

    AVFormatContext *oc = NULL;
    CHECK_AVERROR(avformat_alloc_output_context2(&oc, NULL, NULL, filename));

    for (int i = 0; i < ic->nb_streams; i++) {
      AVStream *os;
      CHECK_NULL(os = avformat_new_stream(oc, NULL));
      CHECK_AVERROR(
          avcodec_parameters_copy(os->codecpar, ic->streams[i]->codecpar));
      os->codecpar->codec_tag = 0;
    }

    if (verbosity >= 1) {
      // 出力ストリームの情報表示
      av_dump_format(oc, 0, filename, 1);
    }

    CHECK_AVERROR(avio_open(&oc->pb, filename, AVIO_FLAG_WRITE));
    CHECK_AVERROR(avformat_write_header(oc, NULL));

    // 最初のパケットを出力
    pkt->pts -= pts_offset;
    pkt->dts -= pts_offset;
    av_packet_rescale_ts(pkt, ic->streams[pkt->stream_index]->time_base,
                         oc->streams[pkt->stream_index]->time_base);
    pkt->pos = -1;
    CHECK_AVERROR(av_interleaved_write_frame(oc, pkt));

    while (!stopping) {
      // 続くフレームの受信
      CHECK_AVERROR(av_read_frame(ic, pkt));

      // パケットがキーフレームでありdurationを経過した場合出力ファイルを閉じる
      if (pkt->stream_index == video_stream_index &&
          pkt->flags & AV_PKT_FLAG_KEY && end_pts <= pkt->pts) {
        break;
      }

      // パケットを出力
      pkt->pts -= pts_offset;
      pkt->dts -= pts_offset;
      av_packet_rescale_ts(pkt, ic->streams[pkt->stream_index]->time_base,
                           oc->streams[pkt->stream_index]->time_base);
      pkt->pos = -1;
      CHECK_AVERROR(av_interleaved_write_frame(oc, pkt));
    }

    // 出力ファイルを閉じる
    CHECK_AVERROR(av_write_trailer(oc));
    avio_closep(&oc->pb);
    avformat_free_context(oc);
    oc = NULL;
  }

  /*
   * 後処理
   */
  av_packet_free(&pkt);
  avformat_free_context(ic);
  av_dict_free(&dict);
  return 0;

error:
  if (oc != NULL) {
    avio_closep(&oc->pb);
    avformat_free_context(oc);
  }
  av_packet_free(&pkt);
  avformat_free_context(ic);
  av_dict_free(&dict);
  return 1;
}
