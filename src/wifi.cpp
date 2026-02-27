#include <./wifi.h>
#include <ESPmDNS.h>
#include <string.h>
#include "./menu/config.h"
#include "WiFiMulti.h"

static const char* TAG = "wifi";

static const bool FAST_WIFI_CONNECT = true;

int Wifi::setup(void) {
  WiFi.setHostname(DEFAULT_HOSTNAME);
  WiFi.softAPsetHostname(DEFAULT_HOSTNAME);

  if (FAST_WIFI_CONNECT) {
    ESP_LOGD(TAG, "quickly connecting to %s with pass %s", DEFAULT_WIFI_SSID,
             DEFAULT_WIFI_PASS);
    WiFi.begin(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      if (WiFi.status() == WL_CONNECT_FAILED) {
        ESP_LOGW(TAG, "connect failed, continuing...");
        break;
      }
    }

    return 0;
  }

  if (DEFAULT_WIFI_SSID != NULL) {
    wifi_multi.addAP(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASS);
  }

  while (wifi_multi.run() != WL_CONNECTED) {
    delay(100);

    if (wifi_multi.run() == WL_CONNECT_FAILED) {
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
