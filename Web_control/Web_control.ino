#include <WiFi.h>
#include <ESP32Servo.h>
#include <FS.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>

// Network credentials
const char* ssid = "..."; // your ssid
const char* password = "..."; // your password

AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// GPIO the servos are attached to
static const int servoPinLeft = 12;
static const int servoPinRight = 13;
Servo servoLeft, servoRight;

// Movement parameters
String directionString = "";
int stepValue = 0;
int expectedStepValue = 0;
int actualStepValue = 0;
const int centerAngle = 100; // Adjust this value for your neutral servo position

int cur_leftServo, cur_rightServo;

// Create a sensor object
Adafruit_MPU6050 mpu;
sensors_event_t a, g;

// Gyroscope sensor deviation
float gyroXerror = 0.07;
float gyroYerror = 0.03;
float gyroZerror = 0.01;

float gyroX, gyroY, gyroZ;

// Gyroscope and daily max values
int up_max = 0;
int down_max = 0;
int left_max = 0;
int right_max = 0;
int last_dir = 0; // 1: up, 2: right, 3: down, 4: left

unsigned long lastTime = 0;
unsigned long currentTime = millis();
unsigned long gyroDelay = 10;
unsigned long previousTime = 0;
const long timeoutTime = 10000;

String getDateString();

void initMPU() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    initMPU();
  }
  Serial.println("MPU6050 Found!");
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
}

// Movement functions
void moveUp(int degrees) {
  // Update expected step based on actual + potential overshoot
  expectedStepValue = abs(int(gyroX)) + 1; // Adjust this value based on your expected overshoot

  Serial.print("Moving Up: ");
  Serial.println(cur_leftServo + degrees);
  Serial.println(cur_rightServo + degrees);

  // Adjust servo positions based on degrees
  servoLeft.write(cur_leftServo + degrees);
  servoRight.write(cur_rightServo + degrees);

  delay(15);
  cur_leftServo += degrees;
  cur_rightServo += degrees;

  // Calculate actual step based on gyroscope
  actualStepValue = abs(int(gyroX));

  // Update expected step based on actual + potential overshoot
  expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

  // Update max value if needed
  if (actualStepValue > up_max) {
    up_max = actualStepValue;
    saveMovementData();
  }
}

void moveDown(int degrees) {
  // Update expected step based on actual + potential overshoot
  expectedStepValue = abs(int(gyroX)) + 1; // Adjust this value based on your expected overshoot

  Serial.print("Moving Down: ");
  Serial.println(cur_leftServo - degrees);
  Serial.println(cur_rightServo - degrees);

  // Adjust servo positions based on degrees
  servoLeft.write(cur_leftServo - degrees);
  servoRight.write(cur_rightServo - degrees);

  delay(15);
  cur_leftServo -= degrees;
  cur_rightServo -= degrees;

  // Calculate actual step based on gyroscope
  actualStepValue = abs(int(gyroX));

  // Update expected step based on actual + potential overshoot
  expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

  // Update max value if needed
  if (actualStepValue < down_max) { // Use "<" for moving down
    down_max = actualStepValue;
    saveMovementData();
  }
}

void moveLeft(int degrees) {
  // Update expected step based on actual + potential overshoot
  expectedStepValue = abs(int(gyroY)) + 1; // Adjust this value based on your expected overshoot

  Serial.print("Moving Left: ");
  Serial.println(cur_leftServo - degrees);
  Serial.println(cur_rightServo + degrees);

  // Adjust servo positions based on degrees
  servoLeft.write(cur_leftServo - degrees);
  servoRight.write(cur_rightServo + degrees);

  delay(15);
  cur_leftServo -= degrees;
  cur_rightServo += degrees;

  // Calculate actual step based on gyroscope
  actualStepValue = abs(int(gyroY));

  // Update expected step based on actual + potential overshoot
  expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

  // Update max value if needed
  if (actualStepValue > left_max) {
    left_max = actualStepValue;
    saveMovementData();
  }
}

void moveRight(int degrees) {
  // Update expected step based on actual + potential overshoot
  expectedStepValue = abs(int(gyroY)) + 1; // Adjust this value based on your expected overshoot

  Serial.print("Moving Right: ");
  Serial.println(cur_leftServo + degrees);
  Serial.println(cur_rightServo - degrees);

  // Adjust servo positions based on degrees
  servoLeft.write(cur_leftServo + degrees);
  servoRight.write(cur_rightServo - degrees);

  delay(15);
  cur_leftServo += degrees;
  cur_rightServo -= degrees;

  actualStepValue = abs(int(gyroY));
  if (actualStepValue < right_max) { // Use "<" for moving right
    right_max = actualStepValue;
    saveMovementData();
  }
}

void centerMotors() {
  Serial.println("Centering Motors");
  servoLeft.write(centerAngle);
  servoRight.write(centerAngle);
  delay(15);
  cur_leftServo = centerAngle;
  cur_rightServo = centerAngle;
  gyroX = 0;
  gyroY = 0;
  expectedStepValue = 0;
  actualStepValue = 0;
}

// Save movement data to JSON
void saveMovementData() {
  String dateString = getDateString();
  String filename = "/data.json";

  File file = SPIFFS.open(filename, FILE_READ);
  DynamicJsonDocument doc(1024);

  if (file) {
    deserializeJson(doc, file);
  }

  doc[dateString]["up_max"] = up_max;
  doc[dateString]["down_max"] = down_max;
  doc[dateString]["left_max"] = left_max;
  doc[dateString]["right_max"] = right_max;

  file.close();
  file = SPIFFS.open(filename, FILE_WRITE);
  serializeJson(doc, file);
  file.close();
  Serial.println("Saved Movement Data");
}

// Helper function (Basic - adjust for your time zone and requirements)
String getDateString() {
  time_t now;
  struct tm* timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  return String(timeinfo->tm_year + 1900) + "-" + String(timeinfo->tm_mon + 1) + "-" + String(timeinfo->tm_mday);
}

String getGyroReadings() {
  mpu.getEvent(&a, &g, &temp);

  float gyroX_temp = g.gyro.x;
  if (abs(gyroX_temp) > gyroXerror) {
    gyroX += gyroX_temp / 50.00;
  }

  float gyroY_temp = g.gyro.y;
  if (abs(gyroY_temp) > gyroYerror) {
    gyroY += gyroY_temp / 70.00;
  }

  String dateString = getDateString();
  String filename = "/data.json";

  File file = SPIFFS.open(filename, FILE_READ);
  DynamicJsonDocument doc(1024);

  if (file) {
    deserializeJson(doc, file);
  }

  String output;

  serializeJson(doc[dateString], output);

  return output;
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();
  initMPU();

  servoRight.attach(servoPinRight);
  servoLeft.attach(servoPinLeft);
  centerMotors();

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/left", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Left handler");

    int paramsNr = request->params();
    String queryString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      queryString = p->value();
    }
    int degrees = queryString.toInt();

    if (last_dir != 4) {
      centerMotors();
      last_dir = 4;
    }
    moveLeft(degrees); // Move by degrees
    request->send(200, "text/plain", "OK");
  });

  server.on("/up", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Up handler");

    int paramsNr = request->params();
    String queryString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      queryString = p->value();
    }
    int degrees = queryString.toInt();

    if (last_dir != 1) {
      centerMotors();
      last_dir = 1;
    }
    moveUp(degrees); // Move by degrees
    request->send(200, "text/plain", "OK");
  });

  server.on("/right", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Right handler");

    int paramsNr = request->params();
    String queryString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      queryString = p->value();
    }
    int degrees = queryString.toInt();

    if (last_dir != 2) {
      centerMotors();
      last_dir = 2;
    }
    moveRight(degrees); // Move by degrees
    request->send(200, "text/plain", "OK");
  });

  server.on("/center", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Center handler");
    centerMotors();
    request->send(200, "text/plain", "OK");
  });

  server.on("/down", HTTP_GET, [](AsyncWebServerRequest * request) {
    Serial.println("Down handler");

    int paramsNr = request->params();
    String queryString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      queryString = p->value();
    }
    int degrees = queryString.toInt();

    if (last_dir != 3) {
      centerMotors();
      last_dir = 3;
    }
    moveDown(degrees); // Replace 10 with the desired degree value
    request->send(200, "text/plain", "OK");
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient * client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.begin();
}

void loop() {
  if ((millis() - lastTime) > gyroDelay) {
    // Send Events to the Web Server with the Sensor Readings
    events.send(getGyroReadings().c_str(), "data_readings", millis());
    int movementDiff = abs(expectedStepValue - actualStepValue);
    if (movementDiff < 0.5 && expectedStepValue != 0) { // Adjust threshold based on your tolerance
      Serial.println("No significant movement detected, centering motors.");
      centerMotors();
    }
    lastTime = millis();
  }
}
