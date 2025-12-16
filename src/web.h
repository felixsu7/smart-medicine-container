#ifndef WEB_H
#define WEB_H

void setup_wifi();
void setup_webserver();
void reconnect_loop();
int testNotify(const char *message);

#endif
