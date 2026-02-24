#ifndef SMC_UI_H
#define SMC_UI_H

#include <time.h>
#include <cstdint>

struct SMC_TouchInput {
  int x;
  int y;
  int z;
  bool phy_primary_pressed;
  bool sec_primary_pressed;
  bool touch_pressed;
};

struct SMC_SMSMessage {
  char* content;
  char* number;
  time_t timestamp;
};

struct SMC_Melody {};

struct SMC_WifiConfig {};

void smc_init_drivers(void);
void smc_preloop(uint32_t delta);

struct SMC_TouchInput smc_input(void);

// interface
int smc_motor_steps(void);
void smc_motor_set_steps(int steps);
bool smc_motor_running(void);

int smc_sms_send(char* message, char* number);
struct SMC_SMSMessage** smc_sms_read(char* message, char* number);
int smc_sms_signal(void);

int smc_wifi_add(struct SMC_WifiConfig);
int smc_wifi_remove(char* name);
int smc_wifi_connect(char* name);
int smc_wifi_disconnect(void);
int smc_wifi_signal(void);

int smc_alarm_buzzer(struct SMC_Melody);
int smc_alarm_buzzer_off(void);

time_t smc_time_get(void);

int smc_battery_percentage(void);
int smc_battery_powermode(void);
int smc_battery_set_powermode(int mode);

int smc_preferences_set(char* key, char* value);
char* smc_preferences_get(char* key, char* value);

time_t smc_general_uptime(void);
char* smc_general_sw_info(void);

// TODO webapp interface

#endif
