/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.01
 * ESP8266 OLED-SSD1306 Clock main source file
 *****************************************************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

#include "secrets.h"
#include "configuration.h"
#include "time-helpers.h"
#include "forecast.h"
#include "display-SSD1306.h"


#define LED_ON()    digitalWrite(LED_BUILTIN, LOW)
#define LED_OFF()   digitalWrite(LED_BUILTIN, HIGH)

Configuration state;
ClockDisplay display;
Forecast forecast(FORECAST_LATITUDE, FORECAST_LONGITUDE, 600); // coordinates must be defined in secrets.h
wl_status_t wl_status = WL_IDLE_STATUS;
ESP8266WebServer server(80);

inline bool net_status_good(wl_status_t status) {
    return status == WL_CONNECTED || status == WL_DISCONNECTED;
}

bool net_initialize() {
    return WiFi.disconnect() && WiFi.mode(WIFI_STA) && WiFi.hostname(HOST_NAME) &&
        WiFi.config(IPAddress(state.stationIP), IPAddress(state.stationGateway),
            IPAddress(state.stationSubnet), IPAddress(state.stationDNS)) &&
        net_status_good(WiFi.begin(WIFI_SSID, WIFI_PASSWORD));
}

void initializeNTP() {
    NtpHelper::initializeNTP(state.timezone, state.daylight,
        state.timeServer1, state.timeServer2, state.timeServer3, state.ntpenabled);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    LED_ON();

    Serial.begin(UART_SPEED);
    Serial.println("\n\nESP8266 OLED-SSD1306 Clock\n=============================\n");

    state.loadStoredConfigurationOrDefaults();

    // Serial.print("Time Server 1: ");
    // Serial.println(state.timeServer1);
    // Serial.print("Time Server 2: ");
    // Serial.println(state.timeServer2);
    // Serial.print("Time Server 3: ");
    // Serial.println(state.timeServer3);

    if (true) {
        initializeNTP();
        Time(2000, 1, 1, 0, 0, 0).setSystemTime();
    }

    display.initialize(state.displayBrightness, state.displayColors);

    Serial.print("Initializing network: ");
    Serial.println(net_initialize() ? "OK" : "FAILED");
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("IP Address:  ");
    Serial.println(IPAddress(state.stationIP));
    Serial.print("Gateway:     ");
    Serial.println(IPAddress(state.stationGateway));
    Serial.print("DNS Address: ");
    Serial.println(IPAddress(state.stationDNS));
    Serial.print("Subnet Mask: ");
    Serial.println(IPAddress(state.stationSubnet));
    Serial.print("SSID Name:   ");
    Serial.println(state.wifiSSID);

    Serial.print("Initializing filesystem: ");
    Serial.println(LittleFS.begin() ? "OK" : "FAILED");

    server.on("/time", HTTP_GET, []() {
        server.send(200, "text/html", Time::now().toString());
    });

    server.on("/status", HTTP_GET, []() {
        String json("{\"date\":\"[DATE]\", \"timezone\":[ZONE], \"daylight\":[DAYL], \"ntpenabled\":[NTPE], \"ntpserver1\":\"[NTP1]\", \"ntpserver2\":\"[NTP2]\", \"ntpserver3\":\"[NTP3]\", \"brightness\":[BRIG], \"colors\":\"[CLRS]\"}");
        json.replace("[DATE]", Time::now().toString());
        json.replace("[ZONE]", "3");
        json.replace("[DAYL]", "0");
        json.replace("[NTPE]", "true");
        json.replace("[NTP1]", "0.pool.ntp.org");
        json.replace("[NTP2]", "1.pool.ntp.org");
        json.replace("[NTP3]", "time.nist.gov");
        json.replace("[BRIG]", String(display.getBrightness()));
        json.replace("[CLRS]", display.getColorScheme());

        server.send(200, "application/json", json);
        Serial.println("Processed GET(/status)");
    });

    server.on("/set-date", HTTP_POST, []() {
        String date = server.arg("date");
        //time_t tt = parse_datetime(date.c_str());
        //bool succ = (tt >= 0) && set_datetime(tt);
        bool succ = Time(date.c_str()).setSystemTime();
        String msg = String(succ ? "Set new date: " : "Date set failed: ") + date;
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/syncronize", HTTP_POST, []() {
        initializeNTP();
        server.send(200, "text/html", "SNTP restarted");
        Serial.println("SNTP restarted");
    });

    server.on("/set-display", HTTP_POST, []() {
        if (display.setBrightnessAndColorScheme(
            server.arg("brightness"), server.arg("colors"))) {

            display.copyBrightnessAndColorScheme(
                &state.displayBrightness, state.displayColors);
        
            const char * msg = "Display scheme updated";
            server.send(200, "text/html", msg);
            Serial.println(msg);
        }
        else {
            const char * msg = "Display scheme update failed";
            server.send(400, "text/html", msg);
            Serial.println(msg);
        }
    });

    server.on("/write-config", HTTP_POST, []() {
        bool succ = state.saveToEEPROM();
        String msg = String("Configuration saved to EEPROM: ") + succ ? "OK" : "FAIL";
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/write-config", HTTP_GET, []() {
        bool succ = state.saveToEEPROM();
        String msg = String("Configuration saved to EEPROM: ") + succ ? "OK" : "FAIL";
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/sync", HTTP_GET, []() {
        initializeNTP();
        server.send(200, "text/html", "SNTP restarted");
    });

    server.serveStatic("/", LittleFS, "/", "no-cache"/*"max-age=3600"*/); //86400
    server.begin();
}

time_t lastsec = -1;
void loop() {
    if (WiFi.status() != wl_status) {
        wl_status = WiFi.status();
        if (wl_status == WL_CONNECTED) {
            Serial.print("Station connected, IP address: ");
            Serial.println(WiFi.localIP());
            LED_OFF();
        }
        else if (wl_status == WL_DISCONNECTED) {
            Serial.println("Station disconnected");
            LED_ON();
        }
        else {
            Serial.print("Station connection state changed: ");
            Serial.println(wl_status);
        }
    }

    time_t sec = time(NULL);    
    if (lastsec != sec) {
        lastsec = sec;
        display.update(sec);

        if (wl_status == WL_CONNECTED && forecast.checkUpdate()) {
            Serial.println(forecast.toString());
            display.updateForecast(forecast);
        }
    }

    if (wl_status == WL_CONNECTED) {
        server.handleClient();
    }

    delay(1);
}