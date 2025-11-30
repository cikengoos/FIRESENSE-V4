#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define DHTPIN 3           // D2 = GPIO3
#define DHTTYPE DHT11
#define FIRE_PIN D8        // D10 = GPIO21

// ---- WIFI ----
const char* ssid = "AHGS";
const char* pass = "12345678";

// ---- BOT CONFIG ----
const char* BOT_NAME = "FireSense-Bot1";
const char* GPS_FAKE = "3.010N,101.502E";

// ---- THRESHOLDS ----
const int TEMP_LIMIT = 45;
const int HUMI_LIMIT = 30;

// ---- RPI5 HTTP ----
String RPI_URL = "http://192.168.154.233:5000/update";

// ---- ThingSpeak ----
String TS_KEY = "2P6N5VC51ISQ74HK";
String TS_URL = "https://api.thingspeak.com/update";

// ---- Sensors ----
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    pinMode(FIRE_PIN, INPUT);
    dht.begin();

    WiFi.begin(ssid, pass);
    Serial.print("WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(200);
        Serial.print(".");
    }
    Serial.println("OK");
}

void loop() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    int fire = digitalRead(FIRE_PIN);

    if (isnan(t) || isnan(h)) {
        Serial.println("DHT fail");
        delay(2000);
        return;
    }

    String status = "SAFE";
    if (fire == 1) status = "DANGEROUS";
    else if (t > TEMP_LIMIT || h < HUMI_LIMIT) status = "ALERT";

    Serial.printf("T=%.1f  H=%.1f  F=%d  %s\n", t, h, fire, status.c_str());

    // ---- SEND TO RPI ----
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(RPI_URL);
        http.addHeader("Content-Type","application/json");

        String json = "{";
        json += "\"name\":\"" + String(BOT_NAME) + "\",";
        json += "\"temp\":" + String(t) + ",";
        json += "\"humi\":" + String(h) + ",";
        json += "\"fire\":" + String(fire) + ",";
        json += "\"status\":\"" + status + "\",";
        json += "\"gps\":\"" + String(GPS_FAKE) + "\"}";
        http.POST(json);
        http.end();
    }

    // ---- SEND TO THINGSPEAK ----
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = TS_URL + "?api_key=" + TS_KEY +
                     "&field1=" + (status=="SAFE"?"1":status=="ALERT"?"2":"3") +
                     "&field2=" + String(t) +
                     "&field3=" + String(h);
        http.begin(url);
        http.GET();
        http.end();
    }

    delay(5000);
}
