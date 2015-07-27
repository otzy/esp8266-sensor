/*
 * user_main.h
 *
 *  Created on: Jul 26, 2015
 *      Author: eugene
 */

#ifndef USER_USER_MAIN_H_
#define USER_USER_MAIN_H_

int thing_vsprintf(char *buff, uint16 buff_size, const char *format, ...);

void initCb(void *arg);

uint64 getThingTime();

#endif /* USER_USER_MAIN_H_ */
