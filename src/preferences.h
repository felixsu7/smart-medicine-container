#ifndef PREFERENCES_H
#define PREFERENCES_H

static const char PREFERENCES_VERSION = 0x00;

struct WiFiConfig {
  char ssid[32];
  char pass[63];
  char priority;
};

class DevicePreferences {
public:
  int setup(void);

  int load_from_fs(void);
  int save_into_fs(void);

  char version = PREFERENCES_VERSION;
  char web_password[32]; // For auth on the webapp.
  WiFiConfig wifi_configs[10];
  int gmt_offset;
  int daylight_offset;
  char notify_url[64];
};

#endif
