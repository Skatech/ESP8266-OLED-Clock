#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include "secrets.h"

#ifndef WIFI_SSID
#error WIFI_SSID constant must be defined in secrets.h file
#endif
#ifndef WIFI_PASSWORD
#error WIFI_PASSWORD constant must be defined in secrets.h file
#endif

#define UART_SPEED 115200
#define HOST_NAME "HALLCLOCK"

#define DEFAULT_IP_ADDRESS IPADDR4_INIT_BYTES(192,168,0,83)
#define DEFAULT_DNS_ADDRESS IPADDR4_INIT_BYTES(192,168,0,1)
#define DEFAULT_GATEWAY IPADDR4_INIT_BYTES(192,168,0,1)
#define DEFAULT_SUBNET IPADDR4_INIT_BYTES(255,255,255,0)

#define COLOR_TICKS   0x080822
#define COLOR_HOURS_N 0x000044
#define COLOR_HOURS_D 0x3333AA
#define COLOR_MINUTES 0xFF0000
#define COLOR_SECONDS 0x001100 

#define STATE_FORMAT_VERSION 0x0100

class Configuration {
    public:
    uint16_t stateCrc16;    // integrity control checksum
    uint16_t stateFormat;   // structure format version
    uint8_t displayBrightness;
    uint8_t timezone;
    uint8_t daylight;
    uint8_t ntpenabled;
    uint32_t stationIP;
    uint32_t stationGateway;
    uint32_t stationSubnet;
    uint32_t stationDNS;
    uint32_t displayColors[5];
    char wifiSSID[16];
    char wifiPassword[16];
    char timeServer1[32];
    char timeServer2[32];
    char timeServer3[32];

    Configuration() {
        memset(this, 0, sizeof(Configuration));
    }

    bool loadStoredConfigurationOrDefaults() {
        if (loadFromEEPROM()) {
            if(checkIntegrity()) {
                if(checkFormatVersion()) {
                    Serial.println("Configuration loaded");
                    return true;
                }
                else Serial.println("Configuration load failed (version mismatch)");
            }
            else Serial.println("Configuration load failed (invalid checksum)");
        }
        else Serial.println("Configuration load failed (EEPROM error)");

        loadDefaults();
        Serial.println("Loaded default configuration");
        return false;
    }

    void loadDefaults() {
        stateFormat = STATE_FORMAT_VERSION;
        displayBrightness = 25;
        timezone = 3;
        daylight = 0;
        ntpenabled = 1;
        stationIP = DEFAULT_IP_ADDRESS;
        stationGateway = DEFAULT_GATEWAY;
        stationSubnet = DEFAULT_SUBNET;
        stationDNS = DEFAULT_DNS_ADDRESS;
        displayColors[0] = COLOR_TICKS;
        displayColors[1] = COLOR_HOURS_N;
        displayColors[2] = COLOR_HOURS_D;
        displayColors[3] = COLOR_MINUTES;
        displayColors[4] = COLOR_SECONDS;
        strcpy(wifiSSID, WIFI_SSID);
        strcpy(wifiPassword, WIFI_PASSWORD);
        strcpy(timeServer1, "0.pool.ntp.org");
        strcpy(timeServer2, "1.pool.ntp.org");
        strcpy(timeServer3, "time.nist.gov");
    }

    bool loadFromEEPROM() {
        EEPROM.begin(sizeof(Configuration));
        EEPROM.get(0, *this);
        return EEPROM.end();
    }

    bool saveToEEPROM() {
        stateFormat = STATE_FORMAT_VERSION;
        stateCrc16 = calculateChecksum();
        EEPROM.begin(sizeof(Configuration));
        EEPROM.put(0, *this);
        return EEPROM.end();
    }

    bool checkFormatVersion() {
        return stateFormat == STATE_FORMAT_VERSION;
    }

    bool checkIntegrity() {
        return stateCrc16 == calculateChecksum();
    }

    uint16_t calculateChecksum() {
        return crc16(((uint8_t *)this) + sizeof(stateCrc16), sizeof(Configuration) - sizeof(stateCrc16));
    }

    static uint16_t crc16(const uint8_t *data, uint16_t size, uint16_t crc = 0xFFFF) {
        while (size--) {
            crc ^= *data++;
            for (uint8_t i = 0; i < 8; ++i) {
                if (crc & 0x01) {
                    crc = (crc >> 1) ^ 0xA001;
                }
                else crc >>= 1;
            }
        }
        return crc;
    }
};