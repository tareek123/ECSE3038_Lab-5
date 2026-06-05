#include <Arduino.h>
#include "DHTesp.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"


// ── DHT22 Setup 
const int DHT_PIN = 15;
DHTesp dhtSensor;

const float TEMP_MIN = -40.0;
const float TEMP_MAX =  80.0;
const float HUM_MIN  =   0.0;
const float HUM_MAX  = 100.0;

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
}


bool validateReadings(float temperature, float humidity) {
 if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[ERROR] Sensor returned NaN — skipping post.");
    return false;
  }
  if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
    Serial.printf("[ERROR] Temperature %.2f°C out of range [%.0f, %.0f] — skipping post.\n",
                  temperature, TEMP_MIN, TEMP_MAX);
    return false;
  }
  if (humidity < HUM_MIN || humidity > HUM_MAX) {
    Serial.printf("[ERROR] Humidity %.1f%% out of range [%.0f, %.0f] — skipping post.\n",
                  humidity, HUM_MIN, HUM_MAX);
    return false;
  }
 return true;
}


void setup() {
    Serial.begin(115200);
    
    dhtSensor.setup(DHT_PIN, DHTesp::DHT22);  // ← add this
    Serial.println("DHT22 initialised on GPIO " + String(DHT_PIN));

    connectToWiFi();
}

// postSensorData is its OWN function — this is where all the JSON and HTTP work happens
void postSensorData(float temperature, float humidity) {
  
  // build the JSON body
  StaticJsonDocument<256> doc;
  doc["sensor_type"] = "DHT22";
  JsonObject values = doc.createNestedObject("values");
  values["temperature"] = serialized(String(temperature, 2));
  values["humidity"]    = serialized(String(humidity, 1));
  String body;
  serializeJson(doc, body);

  // send it
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("api-key", API_KEY);
  int statusCode = http.POST(body);

  if (statusCode > 0) {
    Serial.printf("[POST] Response code: %d\n", statusCode);
  } else {
    Serial.printf("[ERROR] HTTP POST failed: %s\n", http.errorToString(statusCode).c_str());
  }

  http.end();
}

// loop just READS and CALLS postSensorData
void loop() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  Serial.println("Temp: " + String(data.temperature, 2) + "°C");
  Serial.println("Humidity: " + String(data.humidity, 1) + "%");

  if (validateReadings(data.temperature, data.humidity)) {
    postSensorData(data.temperature, data.humidity);  // sends the data
  }
}
