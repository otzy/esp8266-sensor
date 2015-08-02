#ifndef CGIWIFI_H
#define CGIWIFI_H

#include "httpd.h"

int cgiWiFiScan(HttpdConnData *connData);
void tplWlan(HttpdConnData *connData, char *token, void **arg, char *buff_to_send, int buff_len);
int cgiWiFi(HttpdConnData *connData);
int cgiWiFiConnect(HttpdConnData *connData);

#endif
