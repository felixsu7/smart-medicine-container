#ifndef PREFERENCES_H
#define PREFERENCES_H

static const short PREFERENCES_MAGIC = 0x1234;
static const char PREFERENCES_VERSION = 0x00;

struct WiFiConfig {
  char ssid[32];
  char pass[63];
  char priority;
};

struct PreferencesFile {
  short magic = PREFERENCES_MAGIC;
  char version = PREFERENCES_VERSION;
  char web_password[32];
  WiFiConfig wifi_configs[25];
  int gmt_offset;
  int daylight_offset;
  char notify_url[64];
};

int setup_preferences(void);

int preferences_load(void);
int preferences_save(void);

int preferences_set(const struct Preferences *src);
int preferences_get(struct Preferences *dest);
#endif
