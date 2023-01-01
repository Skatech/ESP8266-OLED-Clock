/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.02
 * Time class       - time operations and formatting
 * NtpHelper class  - system NTP service initialization and control
 *****************************************************************************/

#include <Arduino.h>
#include <time.h>
#include <sntp.h>

#pragma once

//#define CONFIG_LWIP_SNTP_UPDATE_DELAY 3600  // seconds between ntp time syncronization, default 3600
#define NOT_A_TIME -1

extern int settimeofday(const struct timeval* tv, const struct timezone* tz);

class Time {
    time_t _ms;

    public:
    Time(const time_t &milliseconds = 0) {
        _ms = milliseconds;
    }

    Time(Time &time) {
        _ms = time._ms;
    }

    Time (int year, int month, int day, int hour, int minute, int second) {
        struct tm t = {0};
        t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
        t.tm_hour = hour; t.tm_min = minute; t.tm_sec = second;
        _ms = mktime(&t);
    }

    // Create time from ISO string "YYYYMMDDTHHMMSSZ"
    Time (const char* input) {
        if (strlen(input) >= 16 && input[8] == 'T' && input[15] == 'Z') {
            struct tm tmm = {0};
            sscanf(input, "%4d%2d%2dT%2d%2d%2dZ",
                &tmm.tm_year, &tmm.tm_mon, &tmm.tm_mday,
                &tmm.tm_hour, &tmm.tm_min, &tmm.tm_sec);
            tmm.tm_year -= 1900;
            tmm.tm_mon -= 1;
            _ms = mktime(&tmm);
        }
        else _ms = NOT_A_TIME;
    }

    const time_t& totalSeconds() const {
        return _ms;
    }

    tm* toDetails() {
        return localtime(&_ms);
    }

    /*
    Return time ISO string "YYYYMMDDTHHMMSSZ"
    %a	Abbreviated weekday name *	                                Thu
    %A	Full weekday name *                                         Thursday
    %b	Abbreviated month name *	                                Aug
    %B	Full month name *	August
    %c	Date and time representation *	                            Thu Aug 23 14:55:02 2001
    %C	Year divided by 100 and truncated to integer (00-99)	    20
    %d	Day of the month, zero-padded (01-31)	                    23
    %D	Short MM/DD/YY date, equivalent to %m/%d/%y	                08/23/01
    %e	Day of the month, space-padded ( 1-31)	                    23
    %F	Short YYYY-MM-DD date, equivalent to %Y-%m-%d	            2001-08-23
    %g	Week-based year, last two digits (00-99)	                01
    %G	Week-based year                                             2001
    %h	Abbreviated month name * (same as %b)	                    Aug
    %H	Hour in 24h format (00-23)	                                14
    %I	Hour in 12h format (01-12)	                                02
    %j	Day of the year (001-366)	                                235
    %m	Month as a decimal number (01-12)	                        08
    %M	Minute (00-59)	                                            55
    %n	New-line character ('\n')	
    %p	AM or PM designation	                                    PM
    %r	12-hour clock time *	                                    02:55:02 pm
    %R	24-hour HH:MM time, equivalent to %H:%M	                    14:55
    %S	Second (00-61)	                                            02
    %t	Horizontal-tab character ('\t')	
    %T	ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S	    14:55:02
    %u	ISO 8601 weekday as number with Monday as 1 (1-7)	        4
    %U	Week number with the first Sunday as the first day of week one (00-53)	33
    %V	ISO 8601 week number (01-53)    	                        34
    %w	Weekday as a decimal number with Sunday as 0 (0-6)	        4
    %W	Week number with the first Monday as the first day of week one (00-53)	34
    %x	Date representation *	                                    08/23/01
    %X	Time representation *	                                    14:55:02
    %y	Year, last two digits (00-99)	                            01
    %Y	Year	                                                    2001
    %z	ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
    If timezone cannot be determined, no characters	+100
    %Z	Timezone name or abbreviation *
    If timezone cannot be determined, no characters	CDT
    %%	A % sign	                                                % */
    String toString(const char* format = "%Y%m%dT%H%M%SZ") const {
        struct tm *t = localtime(&_ms);
        char v[17];
        strftime(v, sizeof(v), format, t);
        return String(v);
    }

    bool setSystemTime() {
        struct timeval tv = { _ms, 0 };
        return settimeofday(&tv, NULL) == 0;
    }

    static Time now() {
        return Time(time(NULL));
    }
};

class NtpHelper {
    public:
    
    static bool isNTPEnabled() {
        return sntp_enabled();
    }

    static void enableNTP(bool enable) {
        if (enable != sntp_enabled()) {
            if (enable) {
                sntp_init();
                Serial.println("sntp_init()");
            }
            else {
                sntp_stop();
                Serial.println("sntp_stop()");
            }
        }
    }

    static void initializeNTP(uint8_t timezone, uint8_t daylight, // TZ_Europe_Moscow (TZ.h)
        const char* timeServer1, const char* timeServer2, const char* timeServer3, bool enable = true) {
        configTime(timezone * 3600, daylight * 3600, timeServer1, timeServer2, timeServer3);
        enableNTP(enable);
    } 
};