#ifndef WEB_H
#define WEB_H

// TODO: seperate wifi from web

int setup_wifi();
int setup_web();

int wifi_reconnect_loop();
int web_test_notify(const char *message);

#endif
