#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int cgiLed(HttpdConnData *connData);
void tplLed(HttpdConnData *connData, char *token, void **arg, char *buff_to_send, int buff_len);
int cgiReadFlash(HttpdConnData *connData);
void tplCounter(HttpdConnData *connData, char *token, void **arg, char *buff_to_send, int buff_len);
void tplConfig(HttpdConnData *connData, char *token, void **arg, char *buff_to_send, int buff_len);
int cgiConfig(HttpdConnData *connData);

#endif
