/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.01
 * open-meteo.com forecast service interaction functions
 *****************************************************************************/

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h> 

#define MINIMAL_REQUEST_REPEAT_INTERVAL 120     // interval between attempts to request service, seconds
#define DEFAULT_WEATHER_UPDATE_INTERVAL 1800    // interval between forecast update, seconds

class Forecast {
    private:
    float _latitude, _longitude, _temperature, _windSpeed, _windDirection;
    int _updateInterval, _weatherCode; 
    time_t _lastUpdate = 0, _lastRequest = 0;

    public:
    Forecast(float latitude, float longitude, int updateInterval = DEFAULT_WEATHER_UPDATE_INTERVAL) {
        _latitude = latitude; _longitude = longitude; _updateInterval = updateInterval;
    }

    float getTemperature() const {
        return _temperature;
    }

    float getWindSpeed() const {
        return _windSpeed;
    }
    
    float getWindDirection() const {
        return _windDirection;
    }

    const char* getWindDirectionWorldSide() const {
        return directionToWorldSide(_windDirection);
    }

    const char* getWeatherDescription() const {
        return weatherCodeDescription(_weatherCode);
    }

    time_t isLoaded() const {
        return _lastUpdate > 0;
    }


    // %W, %w - Weather description, weather code
    // %t     - Temperature, Celsius
    // %S, %s - Wind speed m/s, wind speed km/h
    // %D, %d - Wind world side, wind direction
    String toString(const char* format) const {
        String builder(format);

        while (builder.indexOf("%w") >= 0) {
            builder.replace("%w", String(_weatherCode));
        }

        while (builder.indexOf("%W") >= 0) {
            builder.replace("%W", weatherCodeDescription(_weatherCode));
        }

        while (builder.indexOf("%t") >= 0) {
            builder.replace("%t", String(_temperature, 1));
        }

        while (builder.indexOf("%s") >= 0) {
            builder.replace("%s", String(_windSpeed, 1));
        }

        while (builder.indexOf("%S") >= 0) {
            builder.replace("%S", String(_windSpeed / 3.6F, 1));
        }

        while (builder.indexOf("%d") >= 0) {
            builder.replace("%d", String(_windDirection, 1));
        }

        while (builder.indexOf("%D") >= 0) {
            builder.replace("%D", directionToWorldSide(_windDirection));
        }

        return builder;
    }

    String toString() const {
        return toString("Weather: %W Temp: %t'C Wind: %Sm/s, %D");
    }
    
    bool checkUpdate() {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }

        time_t now = time(NULL);
        if ((_lastUpdate > 0) && (_lastUpdate + _updateInterval > now)) {
            return false;
        }

        if (_lastRequest + MINIMAL_REQUEST_REPEAT_INTERVAL > now) {
            return false;
        }
        
        WiFiClient wifi;
        HTTPClient http;

        _lastRequest = now;
        Serial.print("Forecast updating... ");
        String request("http://api.open-meteo.com/v1/forecast?latitude={LAT}&longitude={LON}&current_weather=true");
        request.replace("{LAT}", String(_latitude));
        request.replace("{LON}", String(_longitude));
        http.begin(wifi, request);

        int code = http.GET();
        if(code == HTTP_CODE_OK) {
            String data = http.getString();
            const char* wdata = strstr(data.c_str(), "{\"temperature\":");    
            float temperature, windSpeed, windDirection;
            int weatherCode;

            if (sscanf(wdata, "{\"temperature\":%f,\"windspeed\":%f,\"winddirection\":%f,\"weathercode\":%d}",
                    &temperature, &windSpeed, &windDirection, &weatherCode) == 4) {
                _temperature = temperature;
                _windSpeed = windSpeed;
                _windDirection = windDirection;
                _weatherCode = weatherCode;
                _lastUpdate = now;
                Serial.println("OK");
                return true;
            }

            Serial.println("FAIL: Invalid data format");
            return false;
        }

        Serial.printf("FAIL: (%d) %s\n", code, http.errorToString(code).c_str());
        return false;
    }

    static const char* directionToWorldSide(float direction) {
        const char* const sides[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        return sides[((int)(direction + 22.5F + 360.0F) % 360 / 45)];
    }
    
    static const char* weatherCodeDescription(int weatherCode) {
        switch (weatherCode) {
            case 0: return "Clear sky";
            case 1: return "Mainly clear";
            case 2: return "Partly cloudy";
            case 3: return "Overcast";
            case 45: return "Fog";
            case 48: return "Rime fog";
            case 51: return "Light drizzle";
            case 53: return "Moderate drizzle";
            case 55: return "Dense drizzle";
            case 56: return "Light freezing drizzle";
            case 57: return "Dense freezing drizzle";
            case 61: return "Slight rain";
            case 63: return "Moderate rain";
            case 65: return "Heavy rain";
            case 66: return "Light freezing rain";
            case 67: return "Heavy freezing rain";
            case 71: return "Slight snow fall";
            case 73: return "Moderate snow fall";
            case 75: return "Heavy snow fall";
            case 77: return "Snow grains";
            case 80: return "Slight rain showers";
            case 81: return "Moderate rain showers";
            case 82: return "Violent rain showers";
            case 85: return "Slight snow showers";
            case 86: return "Heavy snow showers";
            case 95: return "Slight or moderate thunderstorm";
            case 96: return "Thunderstorm with slight hail";
            case 99: return "Thunderstorm with heavy hail";
            default: return "";
        }
    }
};
