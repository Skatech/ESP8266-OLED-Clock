/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.01
 * ESP8266 OLED-SSD1306 Clock main source file
 *****************************************************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266NetBIOS.h>
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

#include "secrets.h"
#include "configuration.h"
#include "DateTime.h"
#include "SNTPControl.h"
#include "forecast.h"
#include "display-SSD1306.h"

#define LED_ON()    digitalWrite(LED_BUILTIN, LOW)
#define LED_OFF()   digitalWrite(LED_BUILTIN, HIGH)

Configuration state;
ClockDisplay display;
ForecastProvider forecast(FORECAST_LATITUDE, FORECAST_LONGITUDE, 600); // coordinates must be defined in secrets.h
wl_status_t wl_status = WL_IDLE_STATUS;
ESP8266WebServer server(WEBUI_PORT);

inline bool net_status_good(wl_status_t status) {
    return status == WL_CONNECTED || status == WL_DISCONNECTED;
}

bool net_initialize() {
    return WiFi.disconnect() && WiFi.mode(WIFI_STA) && WiFi.hostname(HOST_NAME) &&
        WiFi.config(IPAddress(state.stationIP), IPAddress(state.stationGateway),
            IPAddress(state.stationSubnet), IPAddress(state.stationDNS)) &&
        net_status_good(WiFi.begin(WIFI_SSID, WIFI_PASSWORD));
}

bool checkAuthentified() {
    if (server.authenticate(WEBUI_USER, WEBUI_PASSWORD)) {
        return true;
    }
    Serial.println("Authentication requested...");
    server.requestAuthentication(BASIC_AUTH, "ESP-CLOCK-AUTH-REALM", "Authentication failed");
    return false;
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
        SNTPControl::configure(state.timezone, state.daylight,
            state.timeServer1, state.timeServer2, state.timeServer3, state.ntpenabled);

        DateTime(2000, 1, 1, 0, 0, 0).setAsSystemTime();
    }

    // Serial.println(DateTime::now().toTimeString());
    // Serial.println(DateTime::now().toString());
    // Serial.println(forecast.getForecast().getWindWorldSide(true));
    // Serial.println(forecast.getForecast().getWindWorldSide(false));

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
        server.send(200, "text/plain", DateTime::now().toString());
    });

    server.on("/info", HTTP_GET, []() {
        String message = "Status: OK\r\nDate: {DATE}\r\nForecast: {CAST}\r\n";
        message.replace("{DATE}", DateTime::now().toString());
        message.replace("{CAST}", forecast.hasForecastFor(3600)
            ? forecast.getForecast().toString() : String("Unknown"));

        server.send(200, "text/plain", message);
    });

    server.on("/get-state", HTTP_GET, []() {
        String json("{\"date\":\"[DATE]\", \"timezone\":[ZONE], \"daylight\":[DAYL], \"ntpenabled\":[NTPE], \"ntpserver1\":\"[NTP1]\", \"ntpserver2\":\"[NTP2]\", \"ntpserver3\":\"[NTP3]\", \"brightness\":[BRIG], \"colors\":\"[CLRS]\"}");
        json.replace("[DATE]", DateTime::now().toISOString());
        json.replace("[ZONE]", "3");
        json.replace("[DAYL]", "0");
        json.replace("[NTPE]", "true");
        json.replace("[NTP1]", "0.pool.ntp.org");
        json.replace("[NTP2]", "1.pool.ntp.org");
        json.replace("[NTP3]", "time.nist.gov");
        json.replace("[BRIG]", String(display.getBrightness()));
        json.replace("[CLRS]", display.getColorScheme());

        server.send(200, "application/json", json);
        Serial.println("Processed GET(/get-state)");
    });

    server.on("/set-date", HTTP_POST, []() {
        if (checkAuthentified()) {
            DateTime date = DateTime::parseISOString(server.arg("date").c_str());

            if (date.isDateTime() && date.setAsSystemTime()) {
                server.send(200, "text/html", "OK");
                Serial.println("Date changed to " + date.toString());
            }
            else {
                server.send(400, "text/html", "Date set FAILED");
                Serial.println("Date set FAILED");
            }
        }
    });

    server.on("/syncronize", HTTP_POST, []() {
        if (checkAuthentified()) {
            SNTPControl::restart();
            server.send(200, "text/html", "OK");
            Serial.println("SNTP restarted");
        }
    });

    server.on("/set-display", HTTP_POST, []() {
        if (checkAuthentified()) {
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
        }
    });

    server.on("/write-config", HTTP_POST, []() {
        if (checkAuthentified()) {
            if (state.saveToEEPROM()) {
                server.send(200, "text/html", "OK");
                Serial.println("Configuration saved");
            }
            else {
                server.send(400, "text/html", "Configuration save FAILED");
                Serial.println("Configuration save FAILED");
            }
        }
    });

    server.on("/syncronize", HTTP_GET, []() {
        if (checkAuthentified()) {
            SNTPControl::restart();
            server.send(200, "text/html", "SNTP restarted");
            Serial.println("SNTP restarted");
        }
    });

    server.serveStatic("/", LittleFS, "/", "no-cache"/*"max-age=3600"*/);
    server.begin();
    NBNS.begin(WEBUI_HOSTNAME);
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

        if (wl_status == WL_CONNECTED && forecast.pull()) {
            Serial.print(DateTime::now().toTimeString() + "  ");
            Serial.println(forecast.getForecast().toString());
            display.updateForecast(forecast.getForecast());
        }
    }

    if (wl_status == WL_CONNECTED) {
        server.handleClient();
    }

    delay(1);
}