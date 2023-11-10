#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

const char* ssid = "NXNG0917";
const char* password = "========";
const char* serverAddress = "http://10.1.1.94:3000/sensors";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; // Thailand is UTC +7
const int daylightOffset_sec = 0;

WiFiClient client;
HTTPClient http;
DHT dht(D4, DHT11);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  dht.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Initialize NTP client
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%m-%Y %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long timerDelay = 5000;
  if ((millis() - lastTime) > timerDelay) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      Serial.print("Humidity: ");
      Serial.println(humidity);
      Serial.print("Temperature: ");
      Serial.println(temperature);

      String formattedTime = getFormattedTime();
      if (formattedTime.length() > 0) {
        DynamicJsonDocument jsonDocument(256);
        jsonDocument["hum"] = humidity;
        jsonDocument["temp"] = temperature;
        jsonDocument["timestamp"] = formattedTime;

        String jsonData;
        serializeJson(jsonDocument, jsonData);

        http.begin(client, serverAddress);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000);

        int httpResponseCode = http.POST(jsonData);

        if (httpResponseCode > 0) {
          Serial.println("HTTP Response code: " + String(httpResponseCode));
        } else {
          Serial.println("Error on sending POST: " + String(httpResponseCode));
        }
        http.end();
      }
    }
    lastTime = millis();
  }
}
