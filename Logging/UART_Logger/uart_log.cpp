/*
 * uart_log.cpp
 *
 *  Created on: Mar 6, 2019
 *      Author: Ahmed Shokry (a.shokry@riotmicro.com)
 */

#ifdef ENABLE_UART_LOG
#include "mbed.h"
#include <stdarg.h>
#include <string.h>
#include "us_ticker_api.h"

#define MAX_LOG_LEN (200)

RawSerial   pc_usb_serial(USBTX, USBRX, 921600);
Mutex       protect_usb_serial;
char        log_string[MAX_LOG_LEN];


unsigned int get_ms_elapsed(void)
{
    static bool     isFirstTime = true;
    static uint32_t usMax       = 0;
    static uint32_t refTime     = 0;
    unsigned int    usElapsed   = 0;

    if (true == isFirstTime)
    {
        const ticker_info_t*  tickerInfoPtr = us_ticker_get_info();

        usMax       = (0xFFFFFFFF >> (32 - tickerInfoPtr->bits));
        isFirstTime = false;
        refTime     = us_ticker_read();
    }
    else
    {
        uint32_t    curTime     = us_ticker_read();

        usElapsed   = (curTime >= refTime) ? (curTime - refTime) : ((curTime + usMax) - refTime);
        refTime     = curTime;
    }

    return (unsigned int) (usElapsed / 1000);
}


void uart_log(
    const char*     aFrmt,
    const char*     aType,
    const char*     aFileName,
    unsigned int    aLineNbr,
    ...)
{
    int len;
    va_list pArgs;

    protect_usb_serial.lock();

    len = snprintf(log_string, MAX_LOG_LEN, "%8d %s [%s:%d] ", get_ms_elapsed(), aType, aFileName, aLineNbr);
    va_start(pArgs, aFrmt);
    len += vsnprintf((log_string + len), (MAX_LOG_LEN - len), aFrmt, pArgs);
    va_end(pArgs);
    len += snprintf((log_string + len), MAX_LOG_LEN, "\n");
    pc_usb_serial.printf(log_string);

    protect_usb_serial.unlock();
}
#endif
