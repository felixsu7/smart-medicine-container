#ifndef CONFIG_H
#define CONFIG_H

static const char *DEFAULT_WIFI_SSID = "<your wifi name>";
static const char *DEFAULT_WIFI_PASS = "<your wifi password>";

static const char *NOTIFY_URL = "<your ntfy.sh topic url>";

static const int GMT_OFFSET = 60 * 60 * 8; // Philippines is GMT+8
static const int DAYLIGHT_OFFSET = 0;

static const char *NTP_SERVER_PRI = "time.nist.org";
static const char *NTP_SERVER_SEC = "pool.ntp.org";
static const char *NTP_SERVER_TRI = "0.ph.pool.ntp.org";

static const int MAX_ALARMS = 50;
static const char *ALARMS_FILENAME = "/alarms.bin";

#endif
