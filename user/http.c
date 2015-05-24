/*
 * http.c
 *
 *  Created on: May 24, 2015
 *      Author: Evgeny Mazovetskiy
 *
 *  Thanks to http://myesp8266.blogspot.de/2015/03/publish-data-from-your-esp8266-to.html
 */

#include "http.h"
#include "espmissingincludes.h"
#include "c_types.h"
#include "espconn.h"
//#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"


LOCAL struct espconn *pCon = NULL;

http_state_enum http_state = NOT_INITIALIZED;

int http_get_attempts = 0;
int http_get_count = 0;

char payload[HTTP_MAX_GET_SIZE];

int last_request_timestamp = 0;

static void ICACHE_FLASH_ATTR http_send_payload_cb(void *arg){
	  http_get_count++;

	  struct espconn *pespconn = (struct espconn *)arg;

	  http_state = WAITING_RESPONSE;

	  os_printf("http_send_payload_cb:\n");
	  os_printf("\tpayload: %s\n", payload);
	  os_printf("\tpayload length: %d\n", strlen(payload));

	  espconn_sent(pespconn, (uint8*)payload, strlen(payload));
	  os_printf("http_send_payload_cb: sent\n");
}

//TODO call custom callback to process response
static void http_receive_cb(void *arg, char *pdata, unsigned short len){
	os_printf("\nResponse:\n%s\n", pdata);
	http_state = STANDBY;
}

int http_init(){
	pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	if (pCon == NULL)
	{
	        os_printf("pCon ALLOCATION FAIL\r\n");
	        return 0;
	}

	pCon->type = ESPCONN_TCP;
	pCon->state = ESPCONN_NONE;

	pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));

	espconn_regist_connectcb(pCon, http_send_payload_cb);
	espconn_regist_recvcb(pCon, http_receive_cb);

	return 1;
}

http_state_enum http_get_state(){
	//Timeout exceeded, force standby
	if (system_get_time() - last_request_timestamp > HTTP_TIMEOUT){
		http_state = STANDBY;
	}

	return http_state;
}

/**
 *
 */
int http_get(char *ipaddr, int remote_port, char *relative_uri){
	http_get_attempts++;


	if (http_state == NOT_INITIALIZED ){
		http_init();
	}

	if (http_get_state() != STANDBY){
		return -1;
	}

	last_request_timestamp = system_get_time();

	//set up local port
	pCon->proto.tcp->local_port = espconn_port();

	//set up the server remote port
	pCon->proto.tcp->remote_port = remote_port;

	//set up the remote IP
	uint32_t ip = ipaddr_addr(ipaddr); //IP address for thingspeak.com
	os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);

	//set up the local IP
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);
	os_memcpy(pCon->proto.tcp->local_ip, &ipconfig.ip, 4);

//	save url to use it later in a callback
	os_strcpy(payload, relative_uri);

//	os_printf("\t\tuin8 url: %s", (char*)url);

	//Connect to remote server
	int ret = 0;
	ret = espconn_connect(pCon);

	if(ret == 0){
		http_state = WAITING_CONNECTON;
		return 1;
	} else {
		http_state = STANDBY;
		return 0;
	}

}

