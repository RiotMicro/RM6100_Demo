/*
 * Copyright (c) 2019 Riot Micro. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*****************************************************************************************************************************************************
 *
 * I N C L U D E S
 *
 ****************************************************************************************************************************************************/
#include "mbed.h"
#include "CellularContext.h"
#include "AT_CellularDevice.h"
#include "CellularLog.h"
#include "ThisThread.h"

#include "SEGGER_RTT.h"

/*****************************************************************************************************************************************************
 *
 * E X T E R N S
 *
 ****************************************************************************************************************************************************/


/*****************************************************************************************************************************************************
 *
 * M A C R O S
 *
 ****************************************************************************************************************************************************/

#define DEMO_NONE               0
#define DEMO_DWEET_SIGNAL       1

#define LIVE_NETWORK

#define LED_ON      (0)
#define LED_OFF     (1)

#define SGNL_ACTV   (0)
#define SGNL_INACTV (1)

#if (MBED_APP_CONF_TEST_TYPE == DEMO_DWEET_SIGNAL)
  #define MSG_LEN                           (500)
  #define SERVER_NAME                       "www.dweet.io"
#endif

#define SYSTEM_RECOVERY() \
{ \
    LOG_ERROR("SYSTEM RESET..."); \
    ThisThread::sleep_for(2000); \
    NVIC_SystemReset(); \
}

#define BAND3_EARFCN    1440
#define BAND5_EARFCN    2525
#define BAND28_EARFCN   9300
#define BAND2_EARFCN    744
#define BAND8_EARFCN    3606
#define BAND20_EARFCN   6300
#define BAND86_EARFCN   70546

#define RADIO_EARFCN    BAND5_EARFCN

#if (BAND3_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    3
#elif (BAND5_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    5
#elif (BAND28_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    28
#elif (BAND2_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    2
#elif (BAND8_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    8
#elif (BAND20_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    20
#elif (BAND86_EARFCN == RADIO_EARFCN)
  #define RADIO_BAND    86
#endif

#define XSTR(x) STR(x)
#define STR(x)  #x
/*****************************************************************************************************************************************************
 *
 * T Y P E   D E F I N I T I O N S
 *
 ****************************************************************************************************************************************************/

/*****************************************************************************************************************************************************
 *
 * G L O B A L   V A R I A B L E   D E F I N I T I O N S
 *
 ****************************************************************************************************************************************************/

/*****************************************************************************************************************************************************
 *
 * L O C A L   V A R I A B L E   D E F I N I T I O N S
 *
 ****************************************************************************************************************************************************/

/* General. */
DigitalOut  ledsPtr[]  = {  DigitalOut(P0_10, SGNL_INACTV),
                            DigitalOut(P0_22, SGNL_INACTV)  };

DigitalOut modem_chen = DigitalOut(MDMCHEN, SGNL_INACTV);
DigitalOut modem_remap = DigitalOut(MDMREMAP, SGNL_INACTV);
DigitalOut modem_reset = DigitalOut(MDMRST, SGNL_INACTV);

NetworkInterface*   interface   = NULL;

/*****************************************************************************************************************************************************
 *
 * L O C A L   F U N C T I O N   D E F I N I T I O N S
 *
 ****************************************************************************************************************************************************/
#if MBED_CONF_MBED_TRACE_ENABLE
static rtos::Mutex trace_mutex;

static void trace_wait()
{
    trace_mutex.lock();
}

static void trace_release()
{
    trace_mutex.unlock();
}

static char time_st[50];

static char* trace_time(size_t ss)
{
    snprintf(time_st, 49, "[%08llums]", Kernel::get_ms_count());
    return time_st;
}

static void trace_print_function(const char *format)
{
    SEGGER_RTT_printf(0, format);
    SEGGER_RTT_printf(0, "\n");
}

static void trace_open()
{
    mbed_trace_init();
    mbed_trace_prefix_function_set( &trace_time );

    mbed_trace_mutex_wait_function_set(trace_wait);
    mbed_trace_mutex_release_function_set(trace_release);

    mbed_trace_cmdprint_function_set(trace_print_function);
    mbed_trace_print_function_set(trace_print_function);

    mbed_cellular_trace::mutex_wait_function_set(trace_wait);
    mbed_cellular_trace::mutex_release_function_set(trace_release);
}

static void trace_close()
{
    mbed_cellular_trace::mutex_wait_function_set(NULL);
    mbed_cellular_trace::mutex_release_function_set(NULL);

    mbed_trace_free();
}
#endif // #if MBED_CONF_MBED_TRACE_ENABLE

#if defined(LIVE_NETWORK)
/**
 * Connects to the Cellular Network
 */
static nsapi_error_t do_connect()
{
    nsapi_error_t retcode = NSAPI_ERROR_OK;
    uint8_t retry_counter = 0;

    while (interface->get_connection_status() != NSAPI_STATUS_GLOBAL_UP) {
        retcode = interface->connect();
        if (retcode == NSAPI_ERROR_AUTH_FAILURE) {
            LOG_ERROR("Authentication Failure. Exiting application\n");
        } else if (retcode == NSAPI_ERROR_OK) {
            LOG_HI("Connection Established.\n");
        } else if (retry_counter > 3) {
            LOG_ERROR("Fatal connection failure: %d\n", retcode);
        } else {
            LOG_WARN("\n\nCouldn't connect: %d, will retry\n", retcode);
            retry_counter++;
            continue;
        }
        break;
    }
    return retcode;
}

static void status_callback(nsapi_event_t status, intptr_t param)
{
    CellularContext * context = (CellularContext *)interface;
    AT_CellularDevice * device = (AT_CellularDevice *) context->get_device();

    if (status == NSAPI_EVENT_CONNECTION_STATUS_CHANGE) {
        switch(param) {
            case NSAPI_STATUS_LOCAL_UP:
                LOG_LO("Local IP address set (NSAPI_STATUS_LOCAL_UP)!");
                break;
            case NSAPI_STATUS_GLOBAL_UP:
                LOG_LO("Global IP address set (NSAPI_STATUS_GLOBAL_UP)!");
                break;
            case NSAPI_STATUS_CONNECTING:
                LOG_LO("Connecting to network (NSAPI_STATUS_CONNECTING)!");
                break;
            case NSAPI_STATUS_DISCONNECTED:
                LOG_LO("No connection to network (NSAPI_STATUS_DISCONNECTED)!");
                SYSTEM_RECOVERY();
                break;
            default:
                LOG_ERROR("Not supported (%x%X)", param);
                SYSTEM_RECOVERY();
                break;
        }
    }
    else {
        cell_callback_data_t* cb_status = (cell_callback_data_t*)param;
        switch((cellular_connection_status_t)status) {
            case CellularDeviceReady:
            {
                LOG_LO("CellularDeviceReady (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                /* Set the frequency band and earfcn */
                device->_at->at_cmd_discard("+CFUN", "=4");
                device->_at->at_cmd_discard("+BAND", "=" XSTR(RADIO_BAND));
                device->_at->at_cmd_discard("+CFUN", "=1");
                device->_at->at_cmd_discard("+EARFCN", "=" XSTR(RADIO_EARFCN));
                break;
            }
            case CellularSIMStatusChanged:
                LOG_LO("CellularSIMStatusChanged (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularRegistrationStatusChanged:
                LOG_LO("CellularRegistrationStatusChanged (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularRegistrationTypeChanged:
                LOG_LO("CellularRegistrationTypeChanged (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularCellIDChanged:
                LOG_LO("CellularCellIDChanged (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularRadioAccessTechnologyChanged:
                LOG_LO("CellularRadioAccessTechnologyChanged (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularAttachNetwork:
                LOG_LO("CellularAttachNetwork (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularActivatePDPContext:
                LOG_LO("CellularActivatePDPContext (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularSignalQuality:
                LOG_LO("CellularSignalQuality (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularStateRetryEvent:
                LOG_HI("CellularStateRetryEvent (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            case CellularDeviceTimeout:
                //LOG_LO("CellularDeviceTimeout (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                break;
            default:
                LOG_ERROR("Not supported status (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
                SYSTEM_RECOVERY();
                break;
        }
        if (NSAPI_ERROR_OK != cb_status->error)
        {
            LOG_ERROR("Unrecoverable Error: (error=%d) (status=%d) (is_final_try=%d)", cb_status->error, cb_status->status_data, cb_status->final_try);
            SYSTEM_RECOVERY();
        }
    }
}
#endif //#if defined(LIVE_NETWORK)

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */

void blink_led(
    int aCount)
{
    for (int i = 0; i < aCount; i++)
    {
        ledsPtr[0] = ledsPtr[1] = LED_ON;
        ThisThread::sleep_for(200);
        ledsPtr[0] = ledsPtr[1] = LED_OFF;
        ThisThread::sleep_for(400);
    }
}

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */

#if (MBED_APP_CONF_TEST_TYPE == DEMO_DWEET_SIGNAL)
int send_dweet_signal(const char *key, int val)
{
    TCPSocket   socket;

    int result;
    int bytes;
    int retValue    = 0;

    char*   message     = (char*) malloc(MSG_LEN);
    char*   response    = (char*) malloc(MSG_LEN);
    if (NULL == message ||
        NULL == response)
    {
        LOG_HI("ERROR: Failed to allocate buffer(s)");
        retValue    = -1;
        goto FuncExit;
    }
    memset(message, 0, MSG_LEN);
    memset(response, 0, MSG_LEN);

    socket.set_timeout(30000);

    /* create the socket */
    LOG_HI("socket.open...");
    result  = socket.open(interface);
    if (result < 0)
    {
        LOG_WARN("Failed to open TCP Socket ... error = %d", result);
        retValue    = -1;
        goto FuncExit;
    }

    /* connect the socket */
    LOG_HI("socket.connect...");
    result  = socket.connect(SERVER_NAME, 80);
    if (result < 0)
    {
        LOG_WARN("Failed to connect with %s ... error = %d", SERVER_NAME, result);
        retValue    = -1;
        goto FuncExit;
    }

    // compose GET message buffer
    bytes = snprintf(message, MSG_LEN, "GET /dweet/for/" MBED_APP_CONF_DWEET_PAGE "?%s=%d HTTP/1.1\nHost: dweet.io\r\nConnection: close\r\n\r\n", key, val);
    message[bytes] = 0;

    LOG_HI("socket.send...");
    result  = socket.send(message, strlen(message));
    if (result < 0)
    {
        LOG_WARN("Failed to send HTTP request ... error = %d", result);
        retValue    = -1;
        goto FuncExit;
    }

    /* receive the response */
    memset(response, 0, MSG_LEN);

    LOG_HI("socket.recv...");
    result  = socket.recv(response, MSG_LEN - 1);
    if (result < 0)
    {
        LOG_WARN("Failed to receive HTTP response, error = %d", result);
        retValue    = -1;
        goto FuncExit;
    }
    // LOG_HI("Socket received: %s", response);

FuncExit:
    LOG_HI("socket.close...");
    socket.close();
    if (NULL != message)
    {
        free((void*) message);
    }
    if (NULL != response)
    {
        free((void*) response);
    }
    return retValue;
}

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */

void demo_loop(void)
{
    int i       = 0;
    int signal;
    int success = 0;
    int fail    = 0;

    int consecutiveFail = 0;

    while(true)
    {
        ThisThread::sleep_for(1000);
        signal  = (i % 2) ? i : 0;
        if ( 0 != send_dweet_signal("Signal", signal) )
        {
            blink_led(4);
            LOG_WARN("DWEET signal failed");
            fail++;
            consecutiveFail++;

            if (3 <= consecutiveFail)
            {
                LOG_ERROR("A lot of consecutive errors");
                SYSTEM_RECOVERY();
            }
        }
        else
        {
            blink_led(1);
            success++;
            consecutiveFail = 0;
        }
        i++;
        LOG_HI("[[[[ [[[ [[ [ %d Success / %d Failure ] ]] ]]] ]]]]", success, fail);
    }
}

/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */

#elif (MBED_APP_CONF_TEST_TYPE == DEMO_NONE)
void demo_loop(void)
{
    while (true)
    {
        ThisThread::sleep_for(2000);
        LOG_HI("Idle APP...");
        blink_led(2);
    }
}
#endif

/*****************************************************************************************************************************************************
 *
 * G L O B A L   F U N C T I O N   D E F I N I T I O N S
 *
 ****************************************************************************************************************************************************/

int main()
{
#if MBED_CONF_MBED_TRACE_ENABLE
    trace_open();
#endif // #if MBED_CONF_MBED_TRACE_ENABLE

    LOG_HI("RM6100 Demo\n");
    LOG_HI("Built: %s, %s\n", __DATE__, __TIME__);

    ThisThread::sleep_for(100);
    blink_led(1);

    /* Get Modem out of reset */
    modem_chen  = 0;
    modem_remap = 0;
    modem_reset = 0;
    ThisThread::sleep_for(100);
    modem_reset = 1;

    do {
        ThisThread::sleep_for(1000);
        blink_led(3);
    } while(0);

#if (MBED_APP_CONF_TEST_TYPE != DEMO_NONE)
#if defined(LIVE_NETWORK)
#ifdef MBED_CONF_NSAPI_DEFAULT_CELLULAR_PLMN
    LOG_HI("[MAIN], plmn: %s\n", (MBED_CONF_NSAPI_DEFAULT_CELLULAR_PLMN ? MBED_CONF_NSAPI_DEFAULT_CELLULAR_PLMN : "NULL"));
#endif
    LOG_HI("Establishing connection\n");

    interface = CellularContext::get_default_instance();

    MBED_ASSERT(interface);

    /* Attach a status change callback */
    interface->attach(&status_callback);

    // sim pin, apn, credentials and possible plmn are taken automatically from json when using NetworkInterface::set_default_parameters()
    interface->set_default_parameters();

    /* Attempt to connect to a cellular network */
    while (do_connect() != NSAPI_ERROR_OK) {
        LOG_WARN("Could not connect to cellular network .. try again\n");
    }
#endif /*#if defined(LIVE_NETWORK)*/
#endif

    demo_loop();

#if MBED_CONF_MBED_TRACE_ENABLE
    trace_close();
#endif // #if MBED_CONF_MBED_TRACE_ENABLE
    return 0;
}

