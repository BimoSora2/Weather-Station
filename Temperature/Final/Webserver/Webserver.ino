// Import required libraries
#include <Arduino.h>
#include <EEPROM.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <SPIFFS.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <FS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// EEPROM addresses for storing sensor data
#define EEPROM_SIZE 512
#define TEMP_ADDR 0
#define PRESSURE_ADDR 10
#define RAIN_ADDR 20
#define LIGHT_ADDR 30
#define HEIGHT_ADDR 40

// Global Variables
String temp = "ERROR";
String pressure = "ERROR";
String rain = "ERROR";
String light = "ERROR";
String height = "ERROR";

/* Put your SSID & Password */
const char* ssid = "Weather Station";  // Enter SSID here
const char* password = "12345678";  // Enter Password here

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Function to read string from EEPROM
String readStringFromEEPROM(int addr) {
  String data;
  for (int i = 0; i < 10; ++i) {
    data += char(EEPROM.read(addr + i));
  }
  return data;
}

// Function to write string to EEPROM
void writeStringToEEPROM(int addr, const String &data) {
  for (int i = 0; i < 10; ++i) {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.commit();
}

// Function to check if the string read from EEPROM is valid
bool isValidData(const String& data) {
  for (char c : data) {
    if (!isPrintable(c)) {
      return false;
    }
  }
  return true;
}

// Processor function for placeholders
String processor(const String& var) {
  if (var == "TEMP") {
    return temp;
  } else if (var == "PRESSURE") {
    return pressure;
  } else if (var == "RAIN") {
    return rain;
  } else if (var == "LIGHT") {
    return light;
  } else if (var == "HEIGHT") {
    return height;
  }
  return "ERROR";
}

void setup() {
  // Initialize Serial port
  Serial.begin(9600);
  while (!Serial) continue;

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  #ifdef ESP32
    if (!SPIFFS.begin(true)) {
      Serial.println("An error has occurred while mounting SPIFFS");
      return;
    }
  #elif defined(ESP8266)
    if (!SPIFFS.begin()) {
      Serial.println("An error has occurred while mounting SPIFFS");
      return;
    }
  #endif

  Serial.println("SPIFFS mounted successfully");

  // Read initial sensor data from EEPROM
  temp = readStringFromEEPROM(TEMP_ADDR);
  pressure = readStringFromEEPROM(PRESSURE_ADDR);
  rain = readStringFromEEPROM(RAIN_ADDR);
  light = readStringFromEEPROM(LIGHT_ADDR);
  height = readStringFromEEPROM(HEIGHT_ADDR);

  // Validate EEPROM data
  if (!isValidData(temp)) temp = "ERROR";
  if (!isValidData(pressure)) pressure = "ERROR";
  if (!isValidData(rain)) rain = "ERROR";
  if (!isValidData(light)) light = "ERROR";
  if (!isValidData(height)) height = "ERROR";

  // Configure WiFi
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  // Define server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

  // Define routes for static files
  server.on("/assets/css/foundation.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/assets/css/foundation.css", "text/css");
  });
  server.on("/assets/js/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/assets/js/jquery.min.js", "application/javascript");
  });

  // Define routes for image files
  const char* imageFiles[] = {"temp.png", "pressure.png", "rainfall.png", "light.png", "height.png"};
  for (const char* file : imageFiles) {
    String path = "/img/" + String(file);
    server.on(path.c_str(), HTTP_GET, [file](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/img/" + String(file), "image/png");
    });
  }

  // Define routes for sensor data
  server.on("/temp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", temp);
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", pressure);
  });
  server.on("/rain", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", rain);
  });
  server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", light);
  });
  server.on("/height", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", height);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, Serial);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Update sensor readings
  height = String(doc["height"]);
  temp = String(doc["temperature"]);
  pressure = String(doc["pressure"]);
  int range = doc["range"];
  float lux = doc["lux"];

  // Check for error values
  if (height.toInt() == -1) height = "ERROR";
  if (temp.toInt() == -1) temp = "ERROR";
  if (pressure.toInt() == -1) pressure = "ERROR";
  if (range == -1) rain = "ERROR";
  if (lux == -1) light = "ERROR";

  // Map range to rain status
  if (range != -1) {
    switch (range) {
      case 0: rain = "RAIN"; break;
      case 1: rain = "RAIN WARNING"; break;
      case 2: rain = "NOT RAINING"; break;
      default: rain = "ERROR"; break;
    }
  }

  // Map lux to light status
  if (lux != -1) {
    if (lux >= 0 && lux <= 100) {
      light = "DARK";
    } else if (lux > 100 && lux <= 1000) {
      light = "OVERCAST";
    } else if (lux > 1000 && lux <= 10000) {
      light = "BRIGHT";
    } else if (lux > 10000 && lux <= 65535) {
      light = "VERY BRIGHT";
    } else {
      light = "ERROR";
    }
  }

  // Store sensor readings in EEPROM
  writeStringToEEPROM(TEMP_ADDR, temp);
  writeStringToEEPROM(PRESSURE_ADDR, pressure);
  writeStringToEEPROM(RAIN_ADDR, rain);
  writeStringToEEPROM(LIGHT_ADDR, light);
  writeStringToEEPROM(HEIGHT_ADDR, height);

  // Print sensor readings
  Serial.println("Sensor Readings:");
  Serial.println("HEIGHT: " + height);
  Serial.println("TEMPERATURE: " + temp);
  Serial.println("AIR PRESSURE: " + pressure);
  Serial.println("RAINFALL: " + rain);
  Serial.println("LIGHT: " + light);
  Serial.println("-----------------------------------------");
}
