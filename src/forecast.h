/******************************************************************************
 * (c) Skatech Research Lab, 2000-2023.
 * Last change: 2023.01.01
 * open-meteo.com forecast service interaction functions
 *****************************************************************************/

#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define MINIMAL_REQUEST_REPEAT_INTERVAL 60      // seconds between service request attempts 
#define DEFAULT_WEATHER_UPDATE_INTERVAL 1800    // seconds between forecast updates by default

class Forecast {
    private:
    float _temperature, _windspeed, _winddir;
    u8 _weathercode; 
    friend class ForecastProvider;

    public:
    float getTemperature() const {
        return _temperature;
    }

    float getWindSpeed() const {
        return _windspeed;
    }
    
    float getWindDirection() const {
        return _winddir;
    }

    const char* getWindWorldSide() const {
        return getWorldSide(_winddir);
    }

    const char* getWeatherDescription() const {
        return getWeatherCodeDescription(_weathercode);
    }

    // %W, %w - Weather description, weather code
    // %t     - Temperature, Celsius
    // %S, %s - Wind speed m/s, wind speed km/h
    // %D, %d - Wind world side, wind direction
    String toString(const char* format) const {
        String builder(format);
        while (builder.indexOf("%w") >= 0) {
            builder.replace("%w", String(_weathercode));
        }
        while (builder.indexOf("%W") >= 0) {
            builder.replace("%W", getWeatherCodeDescription(_weathercode));
        }
        while (builder.indexOf("%t") >= 0) {
            builder.replace("%t", String(_temperature, 1));
        }
        while (builder.indexOf("%s") >= 0) {
            builder.replace("%s", String(_windspeed, 1));
        }
        while (builder.indexOf("%S") >= 0) {
            builder.replace("%S", String(_windspeed / 3.6F, 1));
        }
        while (builder.indexOf("%d") >= 0) {
            builder.replace("%d", String(_winddir, 1));
        }
        while (builder.indexOf("%D") >= 0) {
            builder.replace("%D", getWorldSide(_winddir));
        }
        return builder;
    }

    String toString() const {
        return toString("Weather:%W %t'C wind:%D %Sm/s");
    }
    
    static const char* getWorldSide(float direction) {
        const char* const sides[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        return sides[((int)(direction + 22.5F + 360.0F) % 360 / 45)];
    }
    
    static const char* getWeatherCodeDescription(u8 weatherCode) {
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
        }
        return NULL;
    }
};

class ForecastProvider {
    private:
    Forecast _forecast;
    time_t _updated, _request;
    float _latitude, _longitude;
    u32 _interval;

    public:
    ForecastProvider(float latitude, float longitude, u32 updateInterval = DEFAULT_WEATHER_UPDATE_INTERVAL) {
        initialize(latitude, longitude, updateInterval);
    }

    void initialize(float latitude, float longitude, u32 updateInterval = DEFAULT_WEATHER_UPDATE_INTERVAL) {
        _latitude = latitude; _longitude = longitude; _interval = updateInterval;
    }

    // Return true when current forecast fresher than given period in seconds
    bool hasForecastFor(u32 lastSeconds) const {
        return _updated > 0 && (_updated + lastSeconds > time(NULL));
    }

    const Forecast& getForecast() const {
        return _forecast;
    }

    // Can be called every tick to check where forecast outdated and refresh
    // Return true only when new forecast received, otherwise false
    bool pull() {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }

        time_t now = time(NULL);
        if (_updated > 0 && (_updated + _interval > now)) {
            return false;
        }

        if (_request + MINIMAL_REQUEST_REPEAT_INTERVAL > now) {
            return false;
        }
        
        _request = now;
        WiFiClient wifi;
        HTTPClient http;
        Serial.print("Forecast updating... ");

        String request("http://api.open-meteo.com/v1/forecast?latitude={LAT}&longitude={LON}&current_weather=true");
        request.replace("{LAT}", String(_latitude));
        request.replace("{LON}", String(_longitude));
        http.begin(wifi, request);

        u32 code = http.GET();
        if(code == HTTP_CODE_OK) {
            String data = http.getString();
            const char* wdata = strstr(data.c_str(), "{\"temperature\":");    
            float temperature, windspeed, winddir;
            int weathercode;

            if (wdata && sscanf(wdata,
                    "{\"temperature\":%f,\"windspeed\":%f,\"winddirection\":%f,\"weathercode\":%d}",
                    &temperature, &windspeed, &winddir, &weathercode) == 4) {
                _forecast._temperature = temperature;
                _forecast._windspeed = windspeed;
                _forecast._winddir = winddir;
                _forecast._weathercode = weathercode;
                _updated = now;
                Serial.println("OK");
                return true;
            }

            Serial.println("FAIL: Invalid data format");
            return false;
        }

        Serial.printf("FAIL: (%d) %s\n", code, http.errorToString(code).c_str());
        return false;
    }
};