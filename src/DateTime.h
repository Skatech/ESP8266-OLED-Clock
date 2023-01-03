/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.02
 * DateTime class - date time operations and formatting helper class
 *****************************************************************************/

#include <Arduino.h>
#include <time.h>

#pragma once

#define NOT_A_TIME -1

extern int settimeofday(const struct timeval* tv, const struct timezone* tz);

class DateTime {
    private:
    time_t _sec;

    public:
    DateTime(const time_t &seconds = 0) {
        _sec = seconds;
    }

    DateTime(const DateTime &time) {
        _sec = time._sec;
    }

    DateTime(u16 year, u8 month, u8 day, u8 hour, u8 minute, u8 second) {
        struct tm t = {0};
        t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
        t.tm_hour = hour; t.tm_min = minute; t.tm_sec = second;
        _sec = mktime(&t);
    }

    bool isDateTime() const {
        return _sec > NOT_A_TIME;
    }

    // returns time since the Epoch (00:00:00 UTC, January 1, 1970), measured in seconds
    const time_t& getSecondsTotal() const {
        return _sec;
    }

    tm* toDetails() {
        return localtime(&_sec);
    }

    /* Return custom formatted time string, default format ISO "YYYYMMDDTHHMMSSZ"
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
    %z	ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100) If timezone cannot be determined, no characters	+100
    %Z	Timezone name or abbreviation * If timezone cannot be determined, no characters	CDT
    %%	A % sign	                                                % */
    String toString(const char* format = "%Y-%m-%d %H:%M:%S") const {
        struct tm *t = localtime(&_sec);
        char v[17];
        strftime(v, sizeof(v), format, t);
        return String(v);
    }

    String toISOString() {
        return toString("%Y%m%dT%H%M%SZ");
    }

    String toDateString() {
        return toString("%Y-%m-%d");
    }

    String toTimeString() {
        return toString("%H:%M:%S");
    }

    bool setAsSystemTime() {
        struct timeval tv = { _sec, 0 };
        return settimeofday(&tv, NULL) == 0;
    }

    static DateTime now() {
        return DateTime(time(NULL));
    }

    // Create time from ISO string format "YYYYMMDDTHHMMSSZ"
    static DateTime parseISOString(const char* input) {
        if (strlen(input) >= 16 && input[8] == 'T' && input[15] == 'Z') {
            struct tm tmm = {0};
            if (sscanf(input, "%4d%2d%2dT%2d%2d%2dZ",
                    &tmm.tm_year, &tmm.tm_mon, &tmm.tm_mday,
                    &tmm.tm_hour, &tmm.tm_min, &tmm.tm_sec) == 6) {
                tmm.tm_year -= 1900;
                tmm.tm_mon -= 1;
                return DateTime(mktime(&tmm));
            }
        }
        return DateTime(NOT_A_TIME);
    }
};
