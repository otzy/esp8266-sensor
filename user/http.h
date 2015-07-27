/*
 * http.h
 *
 *  Created on: May 24, 2015
 *      Author: Evgeny Mazovetskiy
 */

#ifndef USER_HTTP_H_
#define USER_HTTP_H_

#define HTTP_TIMEOUT 15 //seconds
#define HTTP_MAX_GET_SIZE 256
#define HTTP_MAX_HOST_SIZE 128

typedef enum {NOT_INITIALIZED, STANDBY, WAITING_CONNECTON, WAITING_RESPONSE} http_state_enum;
int http_get(char *ipaddr, int remote_port, char *relative_uri);
http_state_enum http_get_state();

#endif /* USER_HTTP_H_ */
