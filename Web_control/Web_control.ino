#include <WiFi.h>
#include <ESP32Servo.h>
#include <FS.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ArduinoJson.h>

// Network credentials
#include "network.ino"

AsyncWebServer server(80);

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

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

double x;
double y;
double z;

int minVal = 265;
int maxVal = 402;

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

unsigned long lastTime = 0;
unsigned long currentTime = millis();
unsigned long gyroDelay = 10;
unsigned long previousTime = 0;
const long timeoutTime = 10000;

// Helper function (Basic - adjust for your time zone and requirements)
String getDateString()
{
  time_t now;
  struct tm *timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  return String(timeinfo->tm_year + 1900) + "-" + String(timeinfo->tm_mon + 1) + "-" + String(timeinfo->tm_mday);
}

void initMPU()
{
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
}

// Movement functions
void centerMotors()
{
  Serial.println("Centering Motors");
  servoLeft.write(centerAngle);
  servoRight.write(centerAngle);
  delay(500);
  cur_leftServo = centerAngle;
  cur_rightServo = centerAngle;
  gyroX = 0;
  gyroY = 0;
  expectedStepValue = 0;
  actualStepValue = 0;
}

void moveUp(int degrees, int repetitions)
{
  for (int i = 0; i < repetitions; i++)
  {
    
    // Update expected step based on actual + potential overshoot
    expectedStepValue = abs(int(gyroX)) + 1; // Adjust this value based on your expected overshoot

    Serial.print("Moving Up: ");
    Serial.println(cur_leftServo + degrees);
    Serial.println(cur_rightServo - degrees);

    // Adjust servo positions based on degrees
    servoLeft.write(cur_leftServo + degrees);
    servoRight.write(cur_rightServo - degrees);

    cur_leftServo += degrees;
    cur_rightServo -= degrees;

    // Calculate actual step based on gyroscope
    actualStepValue = abs(int(gyroX));

    // Update expected step based on actual + potential overshoot
    expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

    Serial.println(actualStepValue);

    // Update max value if needed
    if (actualStepValue > up_max)
    {
      up_max = actualStepValue;
      saveMovementData();
    }
    delay(1000);
    Serial.println("Centering Motors");
    centerMotors();
    yield();
  }
}
void moveDown(int degrees, int repetitions)
{
  for (int i = 0; i < repetitions; i++)
  {
    

    // Update expected step based on actual + potential overshoot
    expectedStepValue = abs(int(gyroX)) + 1; // Adjust this value based on your expected overshoot

    Serial.print("Moving Down: ");
    Serial.println(cur_leftServo - degrees);
    Serial.println(cur_rightServo + degrees);

    // Adjust servo positions based on degrees
    servoLeft.write(cur_leftServo - degrees);
    servoRight.write(cur_rightServo + degrees);

    cur_leftServo -= degrees;
    cur_rightServo += degrees;

    // Calculate actual step based on gyroscope
    actualStepValue = abs(int(gyroX));

    // Update expected step based on actual + potential overshoot
    expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

    // Update max value if needed
    if (actualStepValue > down_max)
    { // Use "<" for moving down
      down_max = actualStepValue;
      saveMovementData();
    }
    delay(1000);
    Serial.println("Centering Motors");
    centerMotors();
    yield();
  }
}

void moveLeft(int degrees, int repetitions)
{
  for (int i = 0; i < repetitions; i++)
  {
    

    // Update expected step based on actual + potential overshoot
    expectedStepValue = abs(int(gyroY)) + 1; // Adjust this value based on your expected overshoot

    Serial.print("Moving Left: ");
    Serial.println(cur_leftServo + (degrees / 2));
    Serial.println(cur_rightServo + (degrees / 2));

    // Adjust servo positions based on degrees
    servoLeft.write(cur_leftServo + (degrees / 2));
    servoRight.write(cur_rightServo + (degrees / 2));

    cur_leftServo += (degrees / 2);
    cur_rightServo += (degrees / 2);

    // Calculate actual step based on gyroscope
    actualStepValue = abs(int(gyroY));

    // Update expected step based on actual + potential overshoot
    expectedStepValue = actualStepValue + 1; // Adjust this value based on your expected overshoot

    // Update max value if needed
    if (actualStepValue > left_max)
    {
      left_max = actualStepValue;
      saveMovementData();
    }
    delay(1000);
    Serial.println("Centering Motors");
    centerMotors();
    yield();
  }
}
void moveRight(int degrees, int repetitions)
{
  for (int i = 0; i < repetitions; i++)
  {
    // Update expected step based on actual + potential overshoot
    expectedStepValue = abs(int(gyroY)) + 1; // Adjust this value based on your expected overshoot

    Serial.print("Moving Right: ");
    Serial.println(cur_leftServo - (degrees / 2));
    Serial.println(cur_rightServo - (degrees / 2));

    // Adjust servo positions based on degrees
    servoLeft.write(cur_leftServo - (degrees / 2));
    servoRight.write(cur_rightServo - (degrees / 2));

    cur_leftServo -= (degrees / 2);
    cur_rightServo -= (degrees / 2);

    actualStepValue = abs(int(gyroY));
    if (actualStepValue > right_max)
    { // Use "<" for moving right
      right_max = actualStepValue;
      saveMovementData();
    }
    delay(1000);
    Serial.println("Centering Motors");
    centerMotors();
    yield();
  }
}

String getGyroReadings()
{

  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 14, true);
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  int xAng = map(AcX, minVal, maxVal, -90, 90);
  int yAng = map(AcY, minVal, maxVal, -90, 90);
  int zAng = map(AcZ, minVal, maxVal, -90, 90);

  x = RAD_TO_DEG * (atan2(-yAng, -zAng) + PI);
  y = RAD_TO_DEG * (atan2(-xAng, -zAng) + PI);
  z = RAD_TO_DEG * (atan2(-yAng, -xAng) + PI);

  gyroX = x;
  gyroY = y;
  gyroZ = z;

  String dateString = getDateString();
  String filename = "/data.json";

  File file = SPIFFS.open(filename, FILE_READ);
  DynamicJsonDocument doc(1024);

  if (file)
  {
    deserializeJson(doc, file);
  }

  String output;

  serializeJson(doc, output);

  return output;
}

// Save movement data to JSON
void saveMovementData()
{
  String dateString = getDateString();
  String filename = "/data.json";

  File file = SPIFFS.open(filename, FILE_READ);
  DynamicJsonDocument doc(1024);

  if (file)
  {
    deserializeJson(doc, file);
  }

  DynamicJsonDocument maxData(1024);

  // Update right movement data
  if (right_max > 0)
  {
    JsonArray rightArr = maxData.createNestedArray("right");
    bool updatedRight = false;
    for (int i = 0; i < doc["right"].size(); i++)
    {
      JsonObject obj = doc["right"][i];
      String date = obj["date"].as<String>();
      int data = obj["data"].as<int>();
      if (date == dateString)
      {
        if (right_max > data)
        {
          obj["data"] = right_max;
          updatedRight = true;
        }
      }
      rightArr.add(obj);
    }
    if (!updatedRight)
    {
      JsonObject newObj = rightArr.createNestedObject();
      newObj["date"] = dateString;
      newObj["data"] = right_max;
    }
  }

  // Update left movement data
  if (left_max > 0)
  {
    JsonArray leftArr = maxData.createNestedArray("left");
    bool updatedLeft = false;
    for (int i = 0; i < doc["left"].size(); i++)
    {
      JsonObject obj = doc["left"][i];
      String date = obj["date"].as<String>();
      int data = obj["data"].as<int>();
      if (date == dateString)
      {
        if (left_max > data)
        {
          obj["data"] = left_max;
          updatedLeft = true;
        }
      }
      leftArr.add(obj);
    }
    if (!updatedLeft)
    {
      JsonObject newObj = leftArr.createNestedObject();
      newObj["date"] = dateString;
      newObj["data"] = left_max;
    }
  }

  // Update down movement data
  if (down_max > 0)
  {
    JsonArray downArr = maxData.createNestedArray("down");
    bool updatedDown = false;
    for (int i = 0; i < doc["down"].size(); i++)
    {
      JsonObject obj = doc["down"][i];
      String date = obj["date"].as<String>();
      int data = obj["data"].as<int>();
      if (date == dateString)
      {
        if (down_max > data)
        {
          obj["data"] = down_max;
          updatedDown = true;
        }
      }
      downArr.add(obj);
    }
    if (!updatedDown)
    {
      JsonObject newObj = downArr.createNestedObject();
      newObj["date"] = dateString;
      newObj["data"] = down_max;
    }
  }

  // Update up movement data
  if (up_max > 0)
  {
    JsonArray upArr = maxData.createNestedArray("up");
    bool updatedUp = false;
    for (int i = 0; i < doc["up"].size(); i++)
    {
      JsonObject obj = doc["up"][i];
      String date = obj["date"].as<String>();
      int data = obj["data"].as<int>();
      if (date == dateString)
      {
        if (up_max > data)
        {
          obj["data"] = up_max;
          updatedUp = true;
        }
      }
      upArr.add(obj);
    }
    if (!updatedUp)
    {
      JsonObject newObj = upArr.createNestedObject();
      newObj["date"] = dateString;
      newObj["data"] = up_max;
    }
  }

  file.close();
  file = SPIFFS.open(filename, FILE_WRITE);
  serializeJson(maxData, file);
  file.close();
  Serial.println("Saved Movement Data");
}

void setup()
{
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();
  initMPU();

  servoRight.attach(servoPinRight);
  servoLeft.attach(servoPinLeft);
  centerMotors();

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.serveStatic("/", SPIFFS, "/");

  server.on("/center", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Center handler");
    centerMotors();
    request->send(200, "text/plain", "OK"); });

  server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Left handler");

    int paramsNr = request->params();
    String angleString;
    String repetitionString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->name() == "degrees") {
        angleString = p->value();
      }
      if (p->name() == "repetitions") {
        repetitionString = p->value();
      }
    }
    int degrees = angleString.toInt();
    int repetitions = repetitionString.toInt();

    Serial.println(degrees);
    Serial.println(repetitions);
    request->send(200, "text/plain", "OK"); 
    moveLeft(degrees, repetitions); // Move by degrees
    
    });

  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Up handler");

    int paramsNr = request->params();
    String angleString;
    String repetitionString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->name() == "degrees") {
        angleString = p->value();
      }
      if (p->name() == "repetitions") {
        repetitionString = p->value();
      }
    }
    int degrees = angleString.toInt();
    int repetitions = repetitionString.toInt();
    request->send(200, "text/plain", "OK");
    moveUp(degrees, repetitions); // Move by degrees
     });

  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Right handler");

    int paramsNr = request->params();
    String angleString;
    String repetitionString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->name() == "degrees") {
        angleString = p->value();
      }
      if (p->name() == "repetitions") {
        repetitionString = p->value();
      }
    }
    int degrees = angleString.toInt();
    int repetitions = repetitionString.toInt();
request->send(200, "text/plain", "OK");
    moveRight(degrees, repetitions); // Move by degrees
     });

  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Down handler");

    int paramsNr = request->params();
    String angleString;
    String repetitionString;
    for (int i = 0; i < paramsNr; i++) {
      AsyncWebParameter *p = request->getParam(i);
      if (p->name() == "degrees") {
        angleString = p->value();
      }
      if (p->name() == "repetitions") {
        repetitionString = p->value();
      }
    }
    int degrees = angleString.toInt();
    int repetitions = repetitionString.toInt();
request->send(200, "text/plain", "OK");
    moveDown(degrees, repetitions); // Replace 10 with the desired degree value
     });

  server.on("/load", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Load handler");
    String data = getGyroReadings();
    request->send(200, "text/plain", data); });

  server.begin();
}

void loop()
{
  if ((millis() - lastTime) > gyroDelay)
  {
    getGyroReadings();
    int movementDiff = abs(expectedStepValue - actualStepValue);
    if (movementDiff < 0.5 && expectedStepValue != 0)
    { // Adjust threshold based on your tolerance
      Serial.println("No significant movement detected, centering motors.");
      centerMotors();
    }
    lastTime = millis();
  }
}
