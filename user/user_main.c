

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "espmissingincludes.h"
#include "osapi.h"

#include "ets_sys.h"
#include "httpd.h"
#include "io.h"
#include "dht22.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "config.h"


int thing_vsprintf(char *buff, uint16 buff_size, const char *format, ...){
	int result;
	va_list args;
	va_start(args, format);
	result = ets_vsnprintf(buff, buff_size-1, format, args);
	va_end(args);
	return result;
}

HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},

	//Config
	{"/config", cgiRedirect, "/config.tpl"},
	{"/config/", cgiRedirect, "/config.tpl"},
	{"/config.tpl", cgiEspFsTemplate, tplConfig}, //show config form
	{"/config.cgi", cgiConfig, NULL},//save config

	{"/flash.bin", cgiReadFlash, NULL},
	{"/led.tpl", cgiEspFsTemplate, tplLed},
	{"/dht22.tpl", cgiEspFsTemplate, tplDHT},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
	{"/led.cgi", cgiLed, NULL},

	//Routines to make the /wifi URL and everything beneath it work.
	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

static ETSTimer t1secTimer;
static uint64 time = 0;
static void t1secTimerCb(void *arg) {
	time++;
}

uint64 getThingTime(){
	return time;
}

static ETSTimer initTimer;

void ICACHE_FLASH_ATTR initCb(void *arg) {
	os_printf("user_init start\n");
	DeviceConfig *config = getConfig();
	os_printf("Config has been read. %d", config->ADCModeFlags);

	ioInit(config);
	led2OnOff(0);
	os_printf("ioInit done\n");

	os_printf("\nReady\n");
	led2OnOff(1);
}


void user_init(void) {
	stdoutInit();
	os_printf("stdoutInit done\n");

	//1 sec timer
	os_timer_disarm(&t1secTimer);
	os_timer_setfn(&t1secTimer, t1secTimerCb, NULL);
	os_timer_arm(&t1secTimer, 1000, 1);

	httpdInit(builtInUrls, 80);
	os_printf("httpdInit done\n");

	//do real init in timer callback cause writing to flash leads to fatal error when called from user_init()
	os_timer_disarm(&initTimer);
	os_timer_setfn(&initTimer, initCb, NULL);
	os_timer_arm(&initTimer, 100, 0);
}
