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

int setup_preferences();

int preferences_load(const char *filename);
int preferences_save(const char *filename);

int preferences_set_web_pass(const char *src);
int preferences_get_web_pass(char *dest);

int preferences_get_wifi_config(const char *ssid, WiFiConfig *dest_config);
int preferences_get_wifi_config(int counter, WiFiConfig *dest_config);
int preferences_list_wifi_configs(WiFiConfig **dest_configs);
int preferences_add_wifi_config(const WiFiConfig *config);
int preferences_remove_wifi_config(const char *ssid);

int preferences_set_gmt(const int gmt);
int preferences_get_gmt();

int preferences_set_daylight(const int daylight);
int preferences_get_daylight();

int preferences_set_notify_url(const char *src);
int preferences_get_notify_url(char *dest);

#endif
