#ifndef SMC_ENDPOINTS_H
#define SMC_ENDPOINTS_H

#include "PsychicHttpServer.h"

void register_endpoints_static(PsychicHttpServer* server);
void register_endpoints_admin(PsychicHttpServer* server);
void register_endpoints_htmx(PsychicHttpServer* server);
void regitser_endpoints_data(PsychicHttpServer* server);

#endif
