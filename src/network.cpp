#include "./config.h"
#include "HTTPClient.h"
#include "PsychicHttpServer.h"
#include "alarm.h"
#include "clock.h"
#include "utils.h"
#include <./network.h>
#include <WiFi.h>
#include <string.h>

static const char *TAG = "web";

PsychicHttpServer webserver(80);

int setup_wifi(void) {
  const char *wifi_ssid = DEFAULT_WIFI_SSID;
  const char *wifi_pass = DEFAULT_WIFI_PASS;

  ESP_LOGD(TAG, "connecting to %s with pass %s", wifi_ssid, wifi_pass);
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);

    if (WiFi.status() == WL_CONNECT_FAILED) {
      ESP_LOGW(TAG, "connect failed, continuing...");
      break;
    }
  }
  return 0;
}

int wifi_reconnect_loop(void) {
  static int lastReconnectCheck;
  if (WiFi.status() != WL_CONNECTED &&
      millis() > lastReconnectCheck + 1000 * 30) {
    ESP_LOGW(TAG, "disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastReconnectCheck = millis();
  }
  return 0;
}

int setup_webserver(Alarms *alarms) {
  webserver.on("/", HTTP_GET, [](PsychicRequest *req, PsychicResponse *res) {
    return res->send("Hello!");
  });

  webserver.on("/test_notify", HTTP_POST,
               [](PsychicRequest *req, PsychicResponse *res) {
                 int code = web_test_notify(req->body().c_str());
                 return res->send(code);
               });

  webserver.on(
      "/alarm", HTTP_POST, [=](PsychicRequest *req, PsychicResponse *res) {
        char reply[sizeof(Alarm) * 3 + 3 + 1];
        struct Alarm alarm;
        memset(reply, 0, sizeof(reply));
        memset(&alarm, 0, sizeof(Alarm));

        if (req->hasParam("name")) {
          // strncat(reply, req->getParam("name")->value().c_str(),
          // sizeof(alarm.name) - 1);
          strncpy(alarm.name, req->getParam("name")->value().c_str(),
                  sizeof(alarm.name) - 1);
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("description")) {
          // strncat(reply, req->getParam("description")->value().c_str(),
          // sizeof(alarm.description) - 1);
          strncpy(alarm.description,
                  req->getParam("description")->value().c_str(),
                  sizeof(alarm.description) - 1);
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("category")) {
          // strncat(reply, req->getParam("category")->value().c_str(), 3);
          alarm.category = (char)req->getParam("category")->value().toInt();
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("flags")) {
          // strncat(reply, req->getParam("flags")->value().c_str(), 3);
          alarm.flags = (char)req->getParam("flags")->value().toInt();
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("days")) {
          // strncat(reply, req->getParam("days")->value().c_str(), 3);
          alarm.days = (char)req->getParam("days")->value().toInt();
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("icon")) {
          // strncat(reply, req->getParam("icon")->value().c_str(), 3);
          alarm.icon = (char)req->getParam("icon")->value().toInt();
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("color")) {
          // strncat(reply, req->getParam("color")->value().c_str(), 4);
          alarm.color = (short)req->getParam("color")->value().toInt();
          // strncat(reply, " ", 1);
        }
        if (req->hasParam("second")) {
          // strncat(reply, req->getParam("second")->value().c_str(), 5);
          alarm.secondMark = req->getParam("second")->value().toInt();
        }

        int idx = alarms->add(&alarm);

        alarms->save_into_fs();

        hexdump(reply, &alarm, sizeof(Alarm));

        sprintf(reply + strlen(reply), "%02d\n", idx);

        return res->send(reply);
      });

  webserver.on(
      "/alarm", HTTP_DELETE, [=](PsychicRequest *req, PsychicResponse *res) {
        // TODO FIXME what?
        if (!req->hasParam("idx")) {
          ESP_LOGW(TAG, "no idx param");
          return res->send(400);
        }

        int err = alarms->set(req->getParam("idx")->value().toInt(), NULL);

        ESP_LOGW(TAG, "err is %d", err);
        ESP_LOGW(TAG, "idx is %d", req->getParam("idx")->value().toInt());

        if (err != 0) {
          return res->send(400);
        }

        assert(alarms->save_into_fs() == 0);

        return res->send(200);
      });

  webserver.on("/earliest_alarm", HTTP_GET,
               [=](PsychicRequest *req, PsychicResponse *res) {
                 struct tm now;
                 Alarm alarm;

                 clock_get(&now);
                 time_t when = alarms->earliest_alarm(&now, &alarm);

                 char reply[50 + 10 + 1];
                 memset(reply, 0, sizeof(reply));

                 sprintf(reply, "%s %ld", alarm.name, when);

                 return res->send(reply);
               });

  webserver.on(
      "/alarms", HTTP_GET, [=](PsychicRequest *req, PsychicResponse *res) {
        struct tm now;
        clock_get(&now);

        for (int i = 0; i < MAX_ALARMS; i++) {
          Alarm alarm;
          if (int err = alarms->get(i, &alarm); err < 0) {
            continue;
          };

          // TODO FIXME this check shouldnt be necessary at all.
          if (alarm.days == 0) {
            continue;
          }

          char msg_part[100];
          sprintf(msg_part, "%d: '%s' days:%02X sec:%d earliest_alarm:%d\n", i,
                  alarm.name, alarm.days, alarm.secondMark,
                  alarms->next_schedule(&alarm, now.tm_wday));

          if (int err = res->sendChunk((uint8_t *)msg_part, strlen(msg_part));
              err != 0) {
            ESP_LOGE(TAG, "sendChunk returned %d", err);
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
    ESP_LOGW(TAG, "not connected");
  }

  WiFiClient client;
  HTTPClient http;

  http.begin(client, NOTIFY_URL);
  int resp = http.POST(message);
  ESP_LOGD(TAG, "resp: %d", resp);
  http.end();
  return resp;
}
