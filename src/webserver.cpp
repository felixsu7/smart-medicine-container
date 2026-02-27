#include <./webserver.h>
#include <ESPmDNS.h>
#include <string.h>
#include "./menu/config.h"
#include "HTTPClient.h"
#include "LittleFS.h"
#include "PsychicHttpServer.h"
#include "clock.h"
#include "endpoints/endpoints.h"
#include "menu/alarm.h"
#include "motor.h"
#include "ui.h"

static const char* TAG = "webserver";

int Webserver::setup(Alarms* alarms) {
  // TODO
  assert(MDNS.begin(DEFAULT_HOSTNAME));
  assert(MDNS.addService("http", "tcp", 80));

  server.on("/test_notify", HTTP_POST,
            [](PsychicRequest* req, PsychicResponse* res) {
              int code = test_notify(req->body().c_str());
              return res->send(code);
            });

  server.on(
      "/alarm", HTTP_POST, [=](PsychicRequest* req, PsychicResponse* res) {
        struct Alarm alarm;
        memset(&alarm, 0, sizeof(Alarm));

        struct tm now;
        Clock::get(&now);

        if (req->hasParam("name")) {
          strncpy(alarm.name, req->getParam("name")->value().c_str(),
                  sizeof(alarm.name) - 1);
        }

        if (req->hasParam("description")) {
          strncpy(alarm.description,
                  req->getParam("description")->value().c_str(),
                  sizeof(alarm.description) - 1);
        }
        if (req->hasParam("category")) {
          alarm.category = (char)req->getParam("category")->value().toInt();
        }
        if (req->hasParam("flags")) {
          alarm.flags = (char)req->getParam("flags")->value().toInt();
        }
        if (req->hasParam("days")) {
          alarm.days = (char)req->getParam("days")->value().toInt();
        }
        if (req->hasParam("icon")) {
          alarm.icon = (char)req->getParam("icon")->value().toInt();
        }
        if (req->hasParam("color")) {
          alarm.color = (short)req->getParam("color")->value().toInt();
        }
        if (req->hasParam("second")) {
          alarm.secondMark = req->getParam("second")->value().toInt();
        }
        if (req->hasParam("in")) {
          alarm.secondMark = req->getParam("in")->value().toInt() +
                             (now.tm_hour * 60 * 60) + (now.tm_min * 60) +
                             now.tm_sec;
        }
        if (req->hasParam("compartment")) {
          alarm.compartment = req->getParam("compartment")->value().toInt();
        }

        ESP_LOGD(TAG, "aaaa %d", alarm.secondMark);

        int idx = alarms->add(&alarm);

        if (idx == -2) {
          return res->send(400);
        }

        assert(alarms->refresh(&now) == 0);

        alarms->save_into_fs();

        char reply[5];
        memset(reply, 0, sizeof(reply));
        snprintf(reply, sizeof(reply), "%d", idx);

        return res->send(reply);
      });

  server.on("/alarm", HTTP_DELETE,
            [=](PsychicRequest* req, PsychicResponse* res) {
              if (!req->hasParam("idx")) {
                return res->send(400);
              }
              int idx = req->getParam("idx")->value().toInt();
              if (idx < 0 || idx >= MAX_ALARMS) {
                return res->send(400);
              }

              int err = alarms->set(idx, NULL);
              ESP_LOGW(TAG, "err is %d", err);
              if (err != 0) {
                return res->send(400);
              }

              struct tm now;
              Clock::get(&now);
              assert(alarms->refresh(&now) == 0);

              assert(alarms->save_into_fs() == 0);

              return res->send(200);
            });

  server.on("/earliest_alarm", HTTP_GET,
            [=](PsychicRequest* req, PsychicResponse* res) {
              struct tm now;
              Clock::get(&now);

              Alarm alarm;
              int idx;
              time_t when = alarms->earliest_alarm(&now, &alarm, &idx);

              char reply[210];
              memset(reply, 0, sizeof(reply));

              snprintf(reply, sizeof(reply),
                       "%d\n%s\n%s\n%d %d %d %d %d %d %ld\n%ld", idx,
                       alarm.name, alarm.description, alarm.category,
                       alarm.flags, alarm.days, alarm.icon, alarm.color,
                       alarm.secondMark, alarm.lastReminded, when);

              return res->send(reply);
            });

  server.on("/alarm", HTTP_GET, [=](PsychicRequest* req, PsychicResponse* res) {
    // TODO FIXME find a way to detect errors without checking if it is zero.
    if (!req->hasParam("idx")) {
      return res->send(400);
    }
    int idx = req->getParam("idx")->value().toInt();
    if (idx < 0 || idx >= MAX_ALARMS) {
      return res->send(400);
    }

    Alarm alarm;
    if (alarms->get(idx, &alarm) != 0) {
      return res->send(404);
    }

    char reply[210];
    memset(reply, 0, sizeof(reply));
    struct tm now;
    Clock::get(&now);
    long when = time(NULL) +
                alarms->next_schedule(
                    &alarm, now.tm_wday,
                    (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec);

    snprintf(reply, sizeof(reply), "%s\n%s\n%d %d %d %d %d %d %ld\n%ld",
             alarm.name, alarm.description, alarm.category, alarm.flags,
             alarm.days, alarm.icon, alarm.color, alarm.secondMark,
             alarm.lastReminded, when);

    return res->send(reply);
  });

  server.on("/alarms", HTTP_GET,
            [=](PsychicRequest* req, PsychicResponse* res) {
              struct tm now;
              Clock::get(&now);

              for (int i = 0; i < MAX_ALARMS; i++) {
                Alarm alarm;
                if (int err = alarms->get(i, &alarm); err < 0) {
                  continue;
                };

                char reply[200];
                memset(reply, 0, sizeof(reply));
                struct tm now;
                Clock::get(&now);
                long when = alarms->next_schedule(
                    &alarm, now.tm_wday,
                    (now.tm_hour * 60 * 60) + (now.tm_min * 60) + now.tm_sec);

                snprintf(reply, sizeof(reply),
                         "<tr><th scope=\"row\">%s</th><td>%d</td><td>%lds</"
                         "td><td>%d</td><td>%lds ago</td></tr>",
                         alarm.name, alarm.compartment, when, alarm.days,
                         time(NULL) - alarm.lastReminded);
                if (int err = res->sendChunk((uint8_t*)reply, strlen(reply));
                    err != 0) {
                  ESP_LOGE(TAG, "sendChunk returned %d", err);
                  res->send(500);
                  return err;
                }
              }

              return res->finishChunking();
            });

  server.on("/attend", HTTP_POST,
            [=](PsychicRequest* req, PsychicResponse* res) {
              if (!req->hasParam("idx")) {
                return res->send(400);
              }

              int idx = req->getParam("idx")->value().toInt();
              Alarm alarm;
              assert(alarms->get(idx, &alarm) == 0);

              if (alarms->attend_idx(idx, time(NULL), 0x00) != 0) {
                return res->send(500);
              }

              return res->send(200);
            });

  server.on("/oneoff", HTTP_POST,
            [=](PsychicRequest* req, PsychicResponse* res) {
              if (!req->hasParam("second")) {
                return res->send(400);
              }

              time_t sec = req->getParam("second")->value().toInt();
              if (sec == 0) {
                return res->send(400);
              }

              time_t when = time(NULL) + sec;

              ESP_LOGD(TAG, "when: %ld", when);

              if (alarms->one_off_ring(when) != 0) {
                return res->send(500);
              }

              return res->send(200);
            });

  server.on("/ring", HTTP_GET, [=](PsychicRequest* req, PsychicResponse* res) {
    int idx;
    time_t when_ring = alarms->ring_in(&idx);
    char name[51];

    if (idx >= 0) {
      Alarm alarm;
      alarms->get(idx, &alarm);
      strcpy(name, alarm.name);
    } else {
      strcpy(name, "One-off alarm");
    }

    char reply[75];
    sprintf(reply, "%s rings in %lds", name, when_ring - time(NULL));
    return res->send(reply);
  });

  server.on("/motor_pos_htmx", HTTP_GET,
            [=](PsychicRequest* req, PsychicResponse* res) {
              char buf[150];
              snprintf(
                  buf, sizeof(buf),
                  "<progress hx-post=\"/motor_pos_htmx\" hx-trigger=\"every "
                  "1s\" hx-swap=\"outerHTML\" value=\"%d\" max=\"4096\" />",
                  smc_motor_steps());
              return res->send(buf);
            });

  server.on("/compartment_pos_htmx", HTTP_GET,
            [=](PsychicRequest* req, PsychicResponse* res) {
              char buf[10];
              snprintf(buf, sizeof(buf), "%d", smc_motor_compartment());
              return res->send(buf);
            });

  server.on("/attend_head_htmx", HTTP_GET,
            [=](PsychicRequest* req, PsychicResponse* res) {
              if (alarms->is_ringing() == -1) {
                return res->send("");
              }

              Alarm alarm;
              alarms->get(alarms->is_ringing(), &alarm);

              char buf[200];
              char alarm_name[16];

              strncpy(alarm_name, alarm.name, sizeof(alarm_name));
              if (strlen(alarm.name) > 15) {
                strncpy(alarm_name + sizeof(alarm_name) - 3, "...", 3);
              }

              snprintf(buf, sizeof(buf),
                       "<button hx-post=\"/attend\" "
                       "hx-vals='{\"idx\":\"%d\"}'>Attend %s</button>",
                       alarms->ringing_idx, alarm_name);
              return res->send(buf);
            });

  server.on("/sim_io", HTTP_POST,
            [=](PsychicRequest* req, PsychicResponse* res) {
              if (!req->hasParam("msg")) {
                return res->send(400);
              }

              int ms = 1000;
              if (req->hasParam("delay")) {
                ms = req->getParam("delay")->value().toInt();
              }

              const char* msg = req->getParam("msg")->value().c_str();

              char buf[250];
              memset(buf, 0, sizeof(buf));

              ESP_LOGD(TAG, "sim TX: %s", msg);
              // sms->IO(msg, buf, sizeof(buf), ms);
              ESP_LOGD(TAG, "sim RX: %s", buf);

              return res->send(200, "text/plain", buf);
            });

  server.on("/spin", HTTP_POST, [=](PsychicRequest* req, PsychicResponse* res) {
    if (!req->hasParam("compartment")) {
      return res->send(400);
    }

    int compartment = req->getParam("compartment")->value().toInt();
    // 0 is the error value of toInt(), sad
    if (compartment <= 0 || compartment > COMPARTMENTS) {
      return res->send(400);
    }
    compartment--;
    smc_motor_move(compartment);

    return res->send(200);
  });

  register_endpoints_static(&server);
  register_endpoints_admin(&server);

  // TODO FIXME WARNING
  server.on("/clearalldata", HTTP_DELETE,
            [](PsychicRequest* req, PsychicResponse* res) {
              assert(LittleFS.format());
              res->send(200);
              esp_restart();
              return 0;
            });

  return server.begin();
}

int Webserver::test_notify(const char* message) {
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
