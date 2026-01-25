#include "LittleFS.h"
#include "PsychicHttpServer.h"
#include "embed.h"
#include "utils.h"

static const char* TAG = "endpoint_static";

int register_endpoints_static(PsychicHttpServer* server) {
  server->on("/", HTTP_GET, [=](PsychicRequest* req, PsychicResponse* res) {
    if (LittleFS.exists("/index.html")) {
      ESP_LOGD(TAG, "exists");
      fs_mutex.lock();
      File file = LittleFS.open("/index.html", FILE_READ);
      if (!file) {
        file.close();
        fs_mutex.unlock();
        ESP_LOGE(TAG, "what?");
        return res->send(200, "text/html", EMBED_INDEX_HTML_DATA);
      }
      char buf[256];
      memset(buf, 0, sizeof(buf));
      int bytes;
      while (true) {
        bytes = file.readBytes(buf, 256);
        if (bytes == 0) {
          break;
        }
        assert(res->sendChunk((uint8_t*)buf, bytes) == 0);
        memset(buf, 0, sizeof(buf));
      }
      file.close();
      fs_mutex.unlock();

      return res->finishChunking();
    } else {
      ESP_LOGD(TAG, "not exist");
      return res->send(200, "text/html", EMBED_INDEX_HTML_DATA);
    }
    return 0;
  });

  server->on(
      "/htmx.js", HTTP_GET, [=](PsychicRequest* req, PsychicResponse* res) {
        res->addHeader("Cache-Control", "public, max-age=604800, immutable");
        return res->send(200, "text/javascript", EMBED_HTMX_JS_DATA);
      });

  server->on(
      "/pico.css", HTTP_GET, [=](PsychicRequest* req, PsychicResponse* res) {
        res->addHeader("Cache-Control", "public, max-age=604800, immutable");
        return res->send(200, "text/css", EMBED_PICO_CSS_DATA);
      });

  return 0;
}
