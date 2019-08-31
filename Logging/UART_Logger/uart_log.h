/*
 * uart_log.h
 *
 *  Created on: Mar 6, 2019
 *      Author: Ahmed Shokry (a.shokry@riotmicro.com)
 */

#ifndef MBED_OS_FEATURES_LOGGING_UART_LOGGER_UART_LOG_H_
#define MBED_OS_FEATURES_LOGGING_UART_LOGGER_UART_LOG_H_

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG_UE_MSG_LEN  (64)

#define LOG_LO(aFrmt, ...)                                                  uart_log(aFrmt, "LO", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define LOG_HI(aFrmt, ...)                                                  uart_log(aFrmt, "HI", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(aFrmt, ...)                                                uart_log(aFrmt, "WR", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN_COND(aCondition, aFrmt, ...)   if (false == (aCondition))  uart_log(aFrmt, "WR", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(aFrmt, ...)                                               uart_log(aFrmt, "ER", __FILENAME__, __LINE__, ##__VA_ARGS__)

void uart_log(
    const char*     aFrmt,
    const char*     aType,
    const char*     aFileName,
    unsigned int    aLineNbr,
    ...);

#endif /* MBED_OS_FEATURES_LOGGING_UART_LOGGER_UART_LOG_H_ */
