#include "./config.h"
#include "HTTPClient.h"
#include "PsychicHttpServer.h"
#include "alarm.h"
#include <./network.h>
#include <WiFi.h>
#include <string.h>

static const char *WEB_TAG = "web";

PsychicHttpServer webserver(80);

int setup_wifi(void) {
  const char *wifi_ssid = DEFAULT_WIFI_SSID;
  const char *wifi_pass = DEFAULT_WIFI_PASS;

  ESP_LOGD(WEB_TAG, "connecting to %s with pass %s", wifi_ssid, wifi_pass);
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);

    if (WiFi.status() == WL_CONNECT_FAILED) {
      ESP_LOGW(WEB_TAG, "connect failed, continuing...");
      break;
    }
  }
  return 0;
}

int wifi_reconnect_loop(void) {
  static int lastReconnectCheck;
  if (WiFi.status() != WL_CONNECTED &&
      millis() > lastReconnectCheck + 1000 * 30) {
    ESP_LOGW(WEB_TAG, "disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastReconnectCheck = millis();
  }
  return 0;
}

int setup_webserver(void) {
  webserver.on("/", HTTP_GET, [](PsychicRequest *req, PsychicResponse *res) {
    return res->send("Hello!");
  });

  webserver.on("/test_notify", HTTP_POST,
               [](PsychicRequest *req, PsychicResponse *res) {
                 int code = web_test_notify(req->body().c_str());
                 return res->send(code);
               });

  // webserver.on(
  // "/alarm", HTTP_POST, [](PsychicRequest *req, PsychicResponse *res) {
  //   const String body = req->body();
  //   Alarm alarm;
  //   // TODO FIXME very unsafe, maybe use json?
  //   int success =
  //       sscanf(body.c_str(),
  //              "name:\"%s\" description:\"%s\" category:%hhu flags:%hhu "
  //              "days:%hhu icon:%hhu color:%hu secondMark:%ld",
  //              alarm.name, alarm.description, alarm.category, alarm.flags,
  //              alarm.days, alarm.icon, alarm.color, alarm.secondMark);
  //
  //   // if (success != 8) {
  //   //   char msg[10];
  //   //   sprintf(msg, "%d", success);
  //   //   return res->send(400, "text/plain", msg);
  //   // }
  //
  //   char msg_part[200];
  //   sprintf(msg_part,
  //           "idx:%d name:%s days:%02X sec:%d category:%02X flags:%02X "
  //           "icon:%02X "
  //           "lastReminded:%ld\n",
  //           -1, alarm.name, 0, alarm.secondMark, alarm.category,
  //           alarm.flags, alarm.icon, alarm.lastReminded);
  //
  //   return res->send(msg_part);
  // });

  webserver.on(
      "/alarms", HTTP_GET, [](PsychicRequest *req, PsychicResponse *res) {
        for (int i = 0; i < MAX_ALARMS; i++) {
          Alarm alarm;
          if (int err = alarm_get(i, &alarm); err < 0) {
            continue;
          };

          char msg_part[200];
          sprintf(msg_part,
                  "idx:%d name:%s days:%02X sec:%d category:%02X flags:%02X "
                  "icon:%02X "
                  "lastReminded:%ld\n",
                  i, alarm.name, 0, alarm.secondMark, alarm.category,
                  alarm.flags, alarm.icon, alarm.lastReminded);

          if (int err = res->sendChunk((uint8_t *)msg_part, strlen(msg_part));
              err != 0) {
            ESP_LOGE(WEB_TAG, "sendChunk returned %d", err);
            res->send(500);
            return err;
          }
        }

        return res->finishChunking();
      });

  return webserver.begin();
}

int web_test_notify(const char *message) {
  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGW(WEB_TAG, "not connected");
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, NOTIFY_URL);
  int resp = http.POST(message);
  ESP_LOGD(WEB_TAG, "resp: %d", resp);
  http.end();
  return resp;
}
