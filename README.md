# RM6100 Example Demo

This is an example based on `mbed-os` cellular APIs that demonstrates RM6100 board.

## Getting started

### Download the application

```sh
$ git clone https://github.com/RiotMicro/RM6100_Demo.git
$ cd RM6100_Demo
$ mbed deploy
$ mbed new .
```

`mbed deploy` step will download the `mbed-os` in a directory with the same name.

### Compiling the application

You may need to use `--clean` option to discard your local changes (use with caution).

Use Mbed CLI commands to generate a binary for the application. For example, in the case of GCC, use the following command:

```sh
$ mbed compile -m RM6100 -t GCC_ARM
```

### Running the application

Download the application binary from `BUILD/RM6100/GCC_ARM/RM6100_Demo.hex` to your board using nrfjprog 
tool as follows:
```sh
$ nrfjprog  --snr $MODEM_JLINK_SN -f NRF52 --program RM6100_Demo.hex --chiperase --verify RM6100_Demo.hex
```

Then you need to reset the board. You can do this using the following command:
```sh
$ nrfjprog --snr $MODEM_JLINK_SN -f NRF52 --reset
```

#### View RTT logs

You can view the RTT logs (coming from the MCU) by running the following commands:

```sh
$ JLinkGDBServer -select USB=$MODEM_JLINK_SN -device NRF52832_XXAA -if SWD -speed 8000 -noir -vd
```
and from another terminal:
```sh
$ JLinkRTTClient
```

You should see an output similar to this:

```
       0 HI [main.cpp:1675] RM6100 Demo
       0 HI [main.cpp:1676] Built: Aug 28 2019, 09:52:04
       0 HI [main.cpp:1678] [MAIN], plmn: NULL
       0 HI [main.cpp:1680] Establishing connection
```

## Changing the application configurations

See the file `mbed_app.json` in the root directory of your application. This file contains all the user specific configurations your application needs (more info [here][0]).


#### Choosing the demo type

In this application, we provide three types of demos:

* **DEMO_NONE:** Gets Modem out of rest then blinks a led indefinitely (MCU is dormant and does not interfere with Modem).
* **DEMO_DWEET_SIGNAL:** Demonstrates the modem connectivity by sending ascending number to 
  a page on [dweet.io][1].

You can choose which demo you want by changing the value of `test-type`
```json
        "test-type": {
            "help": "The test type to build. Options are DEMO_NONE or DEMO_DWEET_SIGNAL",
            "macro_name": "MBED_APP_CONF_TEST_TYPE",
            "value": "DEMO_DWEET_SIGNAL"
        },
```




#### Changing the dweet page

If you are running `DEMO_DWEET_SIGNAL` demo, you can track the device through  [dweet.io/follow/RM6100_DEMO][2]. To change the page name, modify `dweet-page`

```json
        "dweet-page": {
            "help": "Name of dweet.io page which the device will send to it (The page can be viewed at https://dweet.io/follow/PAGE_NAME)",
            "macro_name": "MBED_APP_CONF_DWEET_PAGE",
            "value": "\"RM6100_DEMO\""
        }
```


#### Turning RTT logs on

If you like to enable the logs of the application through SEGGER RTT

```json
    "macros": ["ENABLE_SEGGER_RTT"],
```

#### Turning modem AT echo trace on

If you like details and wish to know about all the AT interactions between the modem and your driver, turn on the modem AT echo trace.

```json
        "cellular.debug-at": true
```

#### Turning on the tracing and trace level

If you like to add more traces or follow the current ones you can turn traces on by changing `mbed-trace.enable` in mbed_app.json

```"target_overrides": {
        "*": {
            "mbed-trace.enable": true,
```

After you have defined `mbed-trace.enable: true`, you can set trace levels by changing value in `trace-level`

 ```"trace-level": {
            "help": "Options are TRACE_LEVEL_ERROR,TRACE_LEVEL_WARN,TRACE_LEVEL_INFO,TRACE_LEVEL_DEBUG",
            "macro_name": "MBED_TRACE_MAX_LEVEL",
            "value": "TRACE_LEVEL_INFO"
        }
```

## References
* [MBed Cellular APIs][3]
* [MBed Configuration System][0]
* [SEGGER RTT (Real Time Transfer)][4]

[0]:https://os.mbed.com/docs/mbed-os/v5.13/reference/configuration.html
[1]: http://dweet.io/
[2]: https://dweet.io/follow/RM6100_DEMO
[3]:https://os.mbed.com/docs/mbed-os/v5.13/apis/cellular-api.html
[4]:https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/
