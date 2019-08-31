/*
 * uart_log.h
 *
 *  Created on: Mar 6, 2019
 *      Author: Ahmed Shokry (a.shokry@riotmicro.com)
 */

#ifndef MBED_OS_FEATURES_LOG_H_
#define MBED_OS_FEATURES_LOG_H_


#if defined(ENABLE_SEGGER_RTT)
#include "SEGGER_RTT.h"
#elif defined(ENABLE_UART_LOG)
#include "uart_log.h"
#endif


#if !defined(LOG_LO)
  #define LOG_LO(...)
#endif

#if !defined(LOG_HI)
  #define LOG_HI(...)
#endif

#if !defined(LOG_WARN)
  #define LOG_WARN(...)
#endif

#if !defined(LOG_WARN_COND)
  #define LOG_WARN_COND(...)
#endif

#if !defined(LOG_ERROR)
  #define LOG_ERROR(...)
#endif

#endif /* MBED_OS_FEATURES_LOG_H_ */
