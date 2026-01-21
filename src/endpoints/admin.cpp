#include "LittleFS.h"
#include "PsychicHttpServer.h"
#include "utils.h"

static const char* TAG = "endpoint_admin";

int register_endpoints_admin(PsychicHttpServer* server) {
  server->on("/upload/index.html", HTTP_POST,
             [](PsychicRequest* req, PsychicResponse* res) {
               assert(req->loadBody() == 0);
               ESP_LOGD(TAG, "body: %d, %s", req->body().length(),
                        req->body().c_str());
               fs_mutex.lock();
               File file = LittleFS.open("/index.html", FILE_WRITE);
               assert(file && !file.isDirectory());
               assert(file.write((const uint8_t*)req->body().c_str(),
                                 req->body().length()) == req->body().length());
               file.close();
               fs_mutex.unlock();
               return res->send(200);
             });

  // TODO FIXME WARNING
  server->on("/clearalldata", HTTP_DELETE,
             [](PsychicRequest* req, PsychicResponse* res) {
               assert(LittleFS.format());
               res->send(200);
               esp_restart();
               return 0;
             });

  return 0;
}
