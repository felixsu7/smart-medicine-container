#ifndef SMC_UI_H
#define SMC_UI_H

#include <time.h>
#include <cstdint>

struct SMC_SMSMessage {
  char* content;
  char* number;
  time_t timestamp;
};

struct SMC_Melody {
  uint8_t* tones;
  size_t tones_len;
  uint16_t sample_rate;
};

struct SMC_WifiConfig {
  char* ssid;
  char* pass;
  int signal_quality;
  int type;  // TODO enum
};

int smc_init_drivers(void);
void smc_loop(void);

// interface
int smc_motor_steps(void);
int smc_motor_compartment(void);
void smc_motor_move(int compartment);
bool smc_motor_running(void);

int smc_sms_send(char* message, char* number);
int smc_sms_list(SMC_SMSMessage** dest, int max_len);
int smc_sms_signal(void);

int smc_wifi_add(struct SMC_WifiConfig* cfg);
int smc_wifi_remove(char* name);
int smc_wifi_connect(char* name);
int smc_wifi_disconnect(void);
int smc_wifi_signal(void);
int smc_wifi_scan(SMC_WifiConfig** dest, int max_len);
int smc_wifi_ap(bool state, char* pass);

void smc_alarm_buzzer_play(struct SMC_Melody);
void smc_alarm_buzzer_off(void);

time_t smc_time_get(void);

int smc_battery_percentage(void);
int smc_battery_powermode(void);
void smc_battery_set_powermode(int mode);

int smc_preferences_load(void);
int smc_preferences_save(void);

int smc_fs_read(const char* path, void* dest, size_t len);
int smc_fs_write(const char* path, const void* src, size_t len);

void smc_data_reset(void);
void smc_device_restart(void);

time_t smc_general_uptime(void);
const char* smc_general_sw_info(void);

// TODO webapp interface and better names
int smc_webapp_local_toggle(bool state);
int smc_webapp_external_endpoints_toggle(bool state);

#endif
