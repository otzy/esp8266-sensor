
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

#include "ets_sys.h"
#include "osapi.h"
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "gpio.h"

#include "http.h"

#define MOVING_AV_WINDOW 3

#define LEDGPIO 5
#define BTNGPIO 0

#define BTN_ADC 2

static ETSTimer resetBtntimer;

static ETSTimer adcBtnTimer;

//static ETSTimer t20secTimer;

static ETSTimer adcToSerialTimer;


int gpio_pin_register[16] = {PERIPHS_IO_MUX_GPIO0_U,
                             PERIPHS_IO_MUX_U0TXD_U,
                             PERIPHS_IO_MUX_GPIO2_U,
                             PERIPHS_IO_MUX_U0RXD_U,
                             PERIPHS_IO_MUX_GPIO4_U,
                             PERIPHS_IO_MUX_GPIO5_U,
                             PERIPHS_IO_MUX_SD_CLK_U,
                             PERIPHS_IO_MUX_SD_DATA0_U,
                             PERIPHS_IO_MUX_SD_DATA1_U,
                             PERIPHS_IO_MUX_SD_DATA2_U,
                             PERIPHS_IO_MUX_SD_DATA3_U,
                             PERIPHS_IO_MUX_SD_CMD_U,
                             PERIPHS_IO_MUX_MTDI_U,
                             PERIPHS_IO_MUX_MTCK_U,
                             PERIPHS_IO_MUX_MTMS_U,
                             PERIPHS_IO_MUX_MTDO_U};

void ICACHE_FLASH_ATTR ioLed(int ena) {
	//gpio_output_set is overkill. ToDo: use better macros
	if (ena) {
		gpio_output_set((1<<LEDGPIO), 0, (1<<LEDGPIO), 0);
	} else {
		gpio_output_set(0, (1<<LEDGPIO), (1<<LEDGPIO), 0);
	}
}

static void ICACHE_FLASH_ATTR resetBtnTimerCb(void *arg) {
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTNGPIO)) {
		resetCnt++;
	} else {
		if (resetCnt>=6) { //3 sec pressed
			wifi_station_disconnect();
			wifi_set_opmode(0x3); //reset to AP+STA mode
			os_printf("Reset to AP mode. Restarting system...\n");
			system_restart();
		} else if (resetCnt>=2) { //1 sec pressed
			//print rtc_time to comport
			os_printf("rtc_time: %d", system_get_rtc_time() * system_rtc_clock_cali_proc());
		}
		resetCnt=0;
	}
}

static void ICACHE_FLASH_ATTR adcBtnTimerCb(void *arg){
	static int resetCnt=0;
	if (!GPIO_INPUT_GET(BTN_ADC)){
		resetCnt++;
	} else {
		if (resetCnt>=2){ //we don't want delay, just filter bounces
			uint16 adc = system_adc_read();
			os_printf("ADC=%d; RTC=%d, SYS_TIME=%d\n", adc, system_get_time(), system_get_rtc_time());
		}
		resetCnt = 0;
	}
}

static uint16 mov_av[MOVING_AV_WINDOW];
uint8 average_cursor = 0;
uint16 last_result;

static void ICACHE_FLASH_ATTR adcToSerialTimerCb(void *arg){
	if (average_cursor == MOVING_AV_WINDOW){
		average_cursor = 0;
	}
	mov_av[average_cursor] = system_adc_read();
	average_cursor++;

	last_result = 0;
	for (int i=0; i<MOVING_AV_WINDOW; i++){
		last_result += mov_av[i];
	}
	last_result = last_result / 3;

	os_printf("%d\n", last_result);
}


//static void ICACHE_FLASH_ATTR t20secTimerCb(void *arg) {
//	//send data to cloud https://api.thingspeak.com/update?key=TS1TFG033WAB7K54&field1=XXXXX
//	char payload[HTTP_MAX_GET_SIZE];
//	int cali = system_rtc_clock_cali_proc();
//	int rtc_time = system_get_rtc_time();
//	os_sprintf(payload, "GET /update?key=TS1TFG033WAB7K54&field1=%d&field2=%d&field3=%d\r\n", cali, rtc_time, rtc_time*cali);
//	os_printf("payload: %s\n", payload);
//	os_printf("http get: %d \n",
//	http_get("184.106.153.149", 80, payload));
//}

void ioInit() {
	
	for (int i=0; i<3; i++){
		mov_av[i] = 0;
	}

	//Set GPIO5 to output mode for relay
	PIN_FUNC_SELECT(gpio_pin_register[LEDGPIO], FUNC_GPIO5);//PERIPHS_IO_MUX_MTCK_U
	PIN_FUNC_SELECT(gpio_pin_register[BTNGPIO], FUNC_GPIO0);
//	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTNGPIO));
	PIN_FUNC_SELECT(gpio_pin_register[BTN_ADC], FUNC_GPIO2);
	gpio_output_set(0, 0, (1<<LEDGPIO), (1<<BTN_ADC)|(1<<BTN_ADC));

	//configure reset button timer
	os_timer_disarm(&resetBtntimer);
	os_timer_setfn(&resetBtntimer, resetBtnTimerCb, NULL);
	os_timer_arm(&resetBtntimer, 500, 1);

	//adcBtnTimer
	os_timer_disarm(&adcBtnTimer);
	os_timer_setfn(&adcBtnTimer, adcBtnTimerCb, NULL);
	os_timer_arm(&adcBtnTimer, 300, 1);

	//adcToSerialTimer
	os_timer_disarm(&adcToSerialTimer);
	os_timer_setfn(&adcToSerialTimer, adcToSerialTimerCb, NULL);
	os_timer_arm(&adcToSerialTimer, 10, 1);

	//20 sec timer
//	os_timer_disarm(&t20secTimer);
//	os_timer_setfn(&t20secTimer, t20secTimerCb, NULL);
//	os_timer_arm(&t20secTimer, 20*1000, 1);
}
