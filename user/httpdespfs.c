/*
Connector to let httpd use the espfs filesystem to serve the files in that.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "espmissingincludes.h"
#include <string.h>
#include <osapi.h>
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "httpd.h"
#include "espfs.h"
#include "httpdespfs.h"


//This is a catch-all cgi function. It takes the url passed to it, looks up the corresponding
//path in the filesystem and if it exists, passes the file through. This simulates what a normal
//webserver would do with static files.
int ICACHE_FLASH_ATTR cgiEspFsHook(HttpdConnData *connData) {
	EspFsFile *file=connData->cgiData;
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		espFsClose(file);
		return HTTPD_CGI_DONE;
	}

	if (file==NULL) {
		//First call to this cgi. Open the file so we can read it.
		file=espFsOpen(connData->url);
		if (file==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}
		connData->cgiData=file;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}

	len=espFsRead(file, buff, 1024);
	if (len>0) espconn_sent(connData->conn, (uint8 *)buff, len);
	if (len!=1024) {
		//We're done.
		espFsClose(file);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}


//cgiEspFsTemplate can be used as a template.

#define TOKEN_SIZE 64

typedef struct {
	EspFsFile *file;
	void *tplArg;
	char token[TOKEN_SIZE];
	int tokenPos;

	char *buffToSend;
	int buffToSendLen;
} TplData;

typedef void (* TplCallback)(HttpdConnData *connData, char *token, void **arg, char *buff_to_send, int buff_len);

//int ICACHE_FLASH_ATTR cgiEspFsTemplate_original(HttpdConnData *connData) {
//	TplData *tpd=connData->cgiData;
//	int len;
//	int x, sp=0;
//	char *e=NULL;
//	char buff[1025];
//
//	if (connData->conn==NULL) {
//		//Connection aborted. Clean up.
//		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
//		espFsClose(tpd->file);
//		os_free(tpd);
//		return HTTPD_CGI_DONE;
//	}
//
//	//TODO what if token resides on the border between 1024 bytes chunks?
//
//	if (tpd==NULL) {
//		//First call to this cgi. Open the file so we can read it.
//		tpd=(TplData *)os_malloc(sizeof(TplData));
//		tpd->file=espFsOpen(connData->url);
//		tpd->tplArg=NULL;
//		tpd->tokenPos=-1;
//		if (tpd->file==NULL) {
//			return HTTPD_CGI_NOTFOUND;
//		}
//		connData->cgiData=tpd;
//		httpdStartResponse(connData, 200);
//		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
//		httpdEndHeaders(connData);
//		return HTTPD_CGI_MORE;
//	}
//
//	len=espFsRead(tpd->file, buff, 1024);
//	if (len>0) {
//		sp=0;
//		e=buff;
//		for (x=0; x<len; x++) {
//			if (tpd->tokenPos==-1) {
//				//Inside ordinary text.
//				if (buff[x]=='%') {
//					//Send raw data up to now
//					if (sp!=0) espconn_sent(connData->conn, (uint8 *)e, sp);
//					sp=0;
//					//Go collect token chars.
//					tpd->tokenPos=0;
//				} else {
//					sp++;
//				}
//			} else {
//				if (buff[x]=='%') {
//					tpd->token[tpd->tokenPos++]=0; //zero-terminate token
//					((TplCallback)(connData->cgiArg))(connData, tpd->token, &tpd->tplArg);
//					//Go collect normal chars again.
//					e=&buff[x+1];
//					tpd->tokenPos=-1;
//				} else {
//					if (tpd->tokenPos<(sizeof(tpd->token)-1)) tpd->token[tpd->tokenPos++]=buff[x];
//				}
//			}
//		}
//	}
//	//Send remaining bit.
//	if (sp!=0) espconn_sent(connData->conn, (uint8 *)e, sp);
//	if (len!=1024) {
//		//We're done.
//		((TplCallback)(connData->cgiArg))(connData, NULL, &tpd->tplArg);
//		espFsClose(tpd->file);
//		return HTTPD_CGI_DONE;
//	} else {
//		//Ok, till next time.
//		return HTTPD_CGI_MORE;
//	}
//}

int ICACHE_FLASH_ATTR cgiEspFsTemplate(HttpdConnData *connData) {
	TplData *tpd=connData->cgiData;
	int len;
	int x=0;
	char buff[1024];
	char one_char;

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		espFsClose(tpd->file);
		os_free(tpd);
		return HTTPD_CGI_DONE;
	}

	//TODO what if token resides on the border between 1024 bytes chunks?

	if (tpd==NULL) {
		//First call to this cgi. Open the file so we can read it.
		tpd=(TplData *)os_malloc(sizeof(TplData));
		tpd->file=espFsOpen(connData->url);
		tpd->tplArg=NULL;
		if (tpd->file==NULL) {
			return HTTPD_CGI_NOTFOUND;
		}
		connData->cgiData=tpd;
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
		httpdEndHeaders(connData);
		return HTTPD_CGI_MORE;
	}

	//Reset our buffers
	for (x=0; x<1024; x++){
		buff[x] = 0;
	}
	for (x=0; x<TOKEN_SIZE; x++){
		tpd->token[x] = 0;
	}
	tpd->tokenPos = -1;
	///////////////////

	len=espFsRead(tpd->file, &one_char, 1);
	for (x = 0; (len>0) && (x<511); x++) {

		if (tpd->tokenPos==-1) {
			//Inside ordinary text.
			if (one_char=='%') {
//				buff[x] = 0; //zero-terminate buffer
				//Go collect token chars.
				tpd->tokenPos=0;
			} else {
				buff[x] = one_char;
			}
		} else {
			if (one_char=='%') {
//				tpd->token[tpd->tokenPos++]=0; //zero-terminate token
				((TplCallback)(connData->cgiArg))(connData, tpd->token, &tpd->tplArg, buff, os_strlen(buff));
				return HTTPD_CGI_MORE;

			} else {
				if (tpd->tokenPos<(TOKEN_SIZE-1)) tpd->token[tpd->tokenPos++]=one_char;
			}
		}

		len=espFsRead(tpd->file, &one_char, 1);
	}
	//We get here only if token was not met. Send the data from buffer
	if(len == 1){
		buff[x] = one_char;
	}
	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
	if (len == 0) {
		//We're done.
		espFsClose(tpd->file);
		return HTTPD_CGI_DONE;
	} else {
		//Ok, till next time.
		return HTTPD_CGI_MORE;
	}
}
