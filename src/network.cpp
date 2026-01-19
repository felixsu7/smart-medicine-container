#include "./config.h"
#include "HTTPClient.h"
#include "LittleFS.h"
#include "PsychicHttpServer.h"
#include "alarm.h"
#include "clock.h"
#include "utils.h"
#include <./network.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <string.h>

static const char *TAG = "web";

int Wifi::setup(void) {
  const char *wifi_ssid = DEFAULT_WIFI_SSID;
  const char *wifi_pass = DEFAULT_WIFI_PASS;

  WiFi.setHostname(DEFAULT_HOSTNAME);
  WiFi.softAPsetHostname(DEFAULT_HOSTNAME);

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

int Wifi::reconnect_loop(void) {
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

int Webserver::setup(Alarms *alarms) {
  // TODO
  assert(MDNS.begin(DEFAULT_HOSTNAME));
  assert(MDNS.addService("http", "tcp", 80));

  server.on("/", HTTP_GET, [](PsychicRequest *req, PsychicResponse *res) {
    return res->send("Hello!");
  });

  server.on("/test_notify", HTTP_POST,
            [](PsychicRequest *req, PsychicResponse *res) {
              int code = test_notify(req->body().c_str());
              return res->send(code);
            });

  server.on(
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

        ESP_LOGD(TAG, "name: %s, days: %d", alarm.name, alarm.days);
        int idx = alarms->add(&alarm);

        ESP_LOGD(TAG, "idx %d", idx);

        if (idx == -2) {
          return res->send(400);
        }

        alarms->save_into_fs();

        struct tm now;
        Clock::get(&now);
        alarms->refresh(&now);

        hexdump(reply, &alarm, sizeof(Alarm));

        sprintf(reply + strlen(reply), "%02d\n", idx);

        return res->send(reply);
      });

  server.on(
      "/alarm", HTTP_DELETE, [=](PsychicRequest *req, PsychicResponse *res) {
        // TODO FIXME what?
        if (!req->hasParam("idx", false)) {
          ESP_LOGW(TAG, "no idx param");
          return res->send(400);
        }

        int err =
            alarms->set(req->getParam("idx", false)->value().toInt(), NULL);

        ESP_LOGW(TAG, "err is %d", err);
        ESP_LOGW(TAG, "idx is %d",
                 req->getParam("idx", false)->value().toInt());

        if (err != 0) {
          return res->send(400);
        }

        assert(alarms->save_into_fs() == 0);

        return res->send(200);
      });

  server.on("/earliest_alarm", HTTP_GET,
            [=](PsychicRequest *req, PsychicResponse *res) {
              struct tm now;
              Alarm alarm;
              int idx;

              Clock::get(&now);
              time_t when = alarms->earliest_alarm(&now, &alarm, &idx);

              char reply[50 + 10 + 1];
              memset(reply, 0, sizeof(reply));

              sprintf(reply, "%d: %s %ld", idx, alarm.name, when);

              return res->send(reply);
            });

  server.on(
      "/alarms", HTTP_GET, [=](PsychicRequest *req, PsychicResponse *res) {
        struct tm now;
        Clock::get(&now);

        for (int i = 0; i < MAX_ALARMS; i++) {
          Alarm alarm;
          if (int err = alarms->get(i, &alarm); err < 0) {
            continue;
          };

          // TODO FIXME this check shouldnt be necessary at all.
          if (alarms->set(-1, &alarm)) {
            continue;
          }

          char msg_part[100];
          int today_sec =
              (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec;
          sprintf(msg_part, "%d: '%s' days:%02X sec:%d earliest_alarm:%d\n", i,
                  alarm.name, alarm.days, alarm.secondMark,
                  alarms->next_schedule(&alarm, now.tm_wday, today_sec));

          if (int err = res->sendChunk((uint8_t *)msg_part, strlen(msg_part));
              err != 0) {
            ESP_LOGE(TAG, "sendChunk returned %d", err);
            res->send(500);
            return err;
          }
        }

        return res->finishChunking();
      });

  server.on("/oneofftest", HTTP_POST,
            [=](PsychicRequest *req, PsychicResponse *res) {
              if (!req->hasParam("when")) {
                return res->send(400);
              }
              time_t when = req->getParam("when")->value().toDouble();
              if (when <= 0) {
                return res->send(400);
              }

              time_t when_actually = time(NULL) + when;

              ESP_LOGD(TAG, "when: %ld", when_actually);

              if (alarms->one_off_ring(when_actually) != 0) {
                return res->send(500);
              }

              return res->send(200);
            });

  server.on("/ringinfo", HTTP_GET,
            [=](PsychicRequest *req, PsychicResponse *res) {
              char reply[50];
              int idx;
              char name[51];

              time_t when_ring = alarms->ring_in(&idx);
              if (idx >= 0) {
                Alarm alarm;
                alarms->get(idx, &alarm);
                strcpy(name, alarm.name);
              } else {
                strcpy(name, "One-off alarm");
              }

              sprintf(reply, "%s rings in %lds", name, when_ring - time(NULL));
              return res->send(reply);
            });

  // TODO FIXME WARNING
  server.on("/clearalldata", HTTP_DELETE,
            [](PsychicRequest *req, PsychicResponse *res) {
              assert(LittleFS.format());
              res->send(200);
              esp_restart();
              return 0;
            });

  return server.begin();
}

int Webserver::test_notify(const char *message) {
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
