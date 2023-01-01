#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "time-helpers.h"
#include "forecast.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define OLED_SCL D1
#define OLED_SDA D2

class ClockDisplay {
    private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C _u8g2;
    uint32_t _colors[5];
    String _forecast;

    public:
    ClockDisplay() : _u8g2(U8G2_R0, OLED_SCL, OLED_SDA) {
    }

    void initialize(uint8_t brightness, uint32_t* colors) {
        _u8g2.begin();
    }

    void clear() {
    }

    uint8_t getBrightness() {
        return 25;
    }
    
    void setBrightness(uint8_t brightness) {
    }

    bool setBrightnessAndColorScheme(String brightnessStr, String colorsStr) {
        return true;
    }


    void setBrightnessAndColorScheme(uint8_t brightness, uint32_t* colors) {
    }

    void copyBrightnessAndColorScheme(uint8_t* brightness, uint32_t* colors) {
    }

    String getColorScheme() {
        return String("0808220000443333aafe01a6001100");
    }

    void updateForecast(const Forecast& forecast) {
        _forecast = forecast.toString("Weather: %W %t'C %D %Sms");
    }

    void update(const Time& now) {
        String tms = now.toString("%H %M");
        String tdt = now.toString("%d %b %y");

        _u8g2.clearBuffer();
        _u8g2.setFont(u8g2_font_logisoso32_tn); //u8g2_font_inb33_mn
        _u8g2.setFontRefHeightExtendedText();
        _u8g2.setDrawColor(1);
        _u8g2.setFontPosTop();
        _u8g2.setFontDirection(0);
        _u8g2.drawStr(14, 0, tms.c_str());
        if (now.totalSeconds() % 2) {
            _u8g2.drawStr(58, 0, ":");
        }

        _u8g2.setFont(u8g2_font_crox5h_tr); // u8g2_font_crox5hb_tr
        _u8g2.setFontRefHeightExtendedText();
        _u8g2.setDrawColor(1);
        _u8g2.setFontPosTop();
        _u8g2.setFontDirection(0);

        unsigned int shift = now.totalSeconds() % (_forecast.length() + 20); // seconds to display date
        if (_forecast.length() > 0 && _forecast.length() > shift) {
            if (shift > 1) {
                String sub = _forecast.substring(shift - 2, shift - 2 + 12);
                _u8g2.drawStr(0, 64 - 20, sub.c_str());
            }
        }
        else _u8g2.drawStr(10, 64 - 20, tdt.c_str());

        _u8g2.sendBuffer();
    }
};