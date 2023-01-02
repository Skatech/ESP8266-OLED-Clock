/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.02
 * SNTPControl class  - helper functions for NTP service configuration and control
 *****************************************************************************/

#include <Arduino.h>
#include <sntp.h>

#pragma once

// seconds between ntp time syncronization, default 3600
//#define CONFIG_LWIP_SNTP_UPDATE_DELAY 3600

class SNTPControl {
    public:
    
    static bool isEnabled() {
        return sntp_enabled();
    }

    static void start() {
        sntp_init();
    }

    static void stop() {
        sntp_stop();
    }

    static int8_t getTimezone() {
        return sntp_get_timezone();
    }

    // timezone, daylight - in hours
    static void configure(int8_t timezone, int8_t daylight, const char* timeServer1,
        const char* timeServer2 = NULL, const char* timeServer3 = NULL, bool enableService = true) {
        configTime(timezone * 3600, daylight * 3600, timeServer1, timeServer2, timeServer3);
        if (enableService == false) {
            stop();
        }
    }
};