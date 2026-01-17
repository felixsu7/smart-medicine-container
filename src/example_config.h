#ifndef CONFIG_H
#define CONFIG_H

static const bool DEBUG = true;

static const char *DEFAULT_WIFI_SSID = "";
static const char *DEFAULT_WIFI_PASS = "";

static const char *NOTIFY_URL = "";

static const int DEFAULT_GMT_OFFSET_SECS = 60 * 60 * 8; // Philippines is GMT+8
static const int DEFAULT_DAYLIGHT_OFFSET_SECS = 0;

static const char *NTP_SERVER_PRI = "time.nist.org";
static const char *NTP_SERVER_SEC = "pool.ntp.org";
static const char *NTP_SERVER_TRI = "0.ph.pool.ntp.org";

static const int MAX_ALARMS = 20;

static const char *ALARMS_PATH = "/alarms";
static const char *PREFERENCES_PATH = "/preferences";

#endif
