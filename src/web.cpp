#include "./config.h"
#include "HTTPClient.h"
#include "PsychicHttpServer.h"
#include <./web.h>
#include <WiFi.h>

static const char *WEB_TAG = "web";

PsychicHttpServer webserver(80);

void setup_wifi() {
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
}

void reconnect_loop() {
  static int lastReconnectCheck;
  if (WiFi.status() != WL_CONNECTED &&
      millis() > lastReconnectCheck + 1000 * 30) {
    ESP_LOGW(WEB_TAG, "disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    lastReconnectCheck = millis();
  }
}

void setup_webserver() {
  // webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
  //   r->send(200, "text/html", "Hello!");
  // });
  // webserver.on("/toggle", HTTP_POST, [](AsyncWebServerRequest *r) {
  //   if (led_state) {
  //     digitalWrite(LED, LOW);
  //     led_state = false;
  //     r->send(200, "text/plain", "Turned off!");
  //   } else {
  //     digitalWrite(LED, HIGH);
  //     led_state = true;
  //     r->send(200, "text/plain", "Turned on!");
  //   }
  // });
  //
  // webserver.begin();
}

int testNotify(const char *message) {
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
