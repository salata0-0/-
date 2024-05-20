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
const int centerAngle = 100; // Adjust this value for your neutral servo position
int cur_leftServo, cur_rightServo;
int expectedStepValue = 0;
int actualStepValue = 0;

int16_t AcX, AcY, AcZ;
double x, y, z;
int minVal = 265;
int maxVal = 402;

// Gyroscope and daily max values
int up_max = 0;
int down_max = 0;
int left_max = 0;
int right_max = 0;

unsigned long lastTime = 0;
unsigned long gyroDelay = 10;

// Helper function (Basic - adjust for your time zone and requirements)
String getDateString() {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
  return String(buffer);
}

void initMPU()
{
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

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
    yield(); // Reset the watchdog timer
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
}

void centerMotors()
{
  Serial.println("Centering Motors");
  servoLeft.write(centerAngle);
  servoRight.write(centerAngle);
  delay(500);
  cur_leftServo = centerAngle;
  cur_rightServo = centerAngle;
  yield();
}

void move(int degrees, int repetitions, void (*moveFunc)(int))
{
  for (int i = 0; i < repetitions; i++)
  {
    moveFunc(degrees);
    delay(1000);
    Serial.println("Centering Motors");
    centerMotors();
  }
}

void moveUp(int degrees)
{
  expectedStepValue = abs(int(x)) + 1;
  Serial.print("Moving Up: ");
  Serial.println(cur_leftServo + degrees);
  Serial.println(cur_rightServo - degrees);

  servoLeft.write(cur_leftServo + degrees);
  servoRight.write(cur_rightServo - degrees);

  cur_leftServo += degrees;
  cur_rightServo -= degrees;
  actualStepValue = abs(int(x));
  expectedStepValue = actualStepValue + 1;

  if (actualStepValue > up_max)
  {
    up_max = actualStepValue;
    saveMovementData();
  }
  yield();
}

void moveDown(int degrees)
{
  expectedStepValue = abs(int(x)) + 1;
  Serial.print("Moving Down: ");
  Serial.println(cur_leftServo - degrees);
  Serial.println(cur_rightServo + degrees);

  servoLeft.write(cur_leftServo - degrees);
  servoRight.write(cur_rightServo + degrees);

  cur_leftServo -= degrees;
  cur_rightServo += degrees;
  actualStepValue = abs(int(x));
  expectedStepValue = actualStepValue + 1;

  if (actualStepValue > down_max)
  {
    down_max = actualStepValue;
    saveMovementData();
  }
  yield();
}

void moveLeft(int degrees)
{
  expectedStepValue = abs(int(y)) + 1;
  Serial.print("Moving Left: ");
  Serial.println(cur_leftServo + (degrees / 2));
  Serial.println(cur_rightServo + (degrees / 2));

  servoLeft.write(cur_leftServo + (degrees / 2));
  servoRight.write(cur_rightServo + (degrees / 2));

  cur_leftServo += (degrees / 2);
  cur_rightServo += (degrees / 2);
  actualStepValue = abs(int(y));
  expectedStepValue = actualStepValue + 1;

  if (actualStepValue > left_max)
  {
    left_max = actualStepValue;
    saveMovementData();
  }
  yield();
}

void moveRight(int degrees)
{
  expectedStepValue = abs(int(y)) + 1;
  Serial.print("Moving Right: ");
  Serial.println(cur_leftServo - (degrees / 2));
  Serial.println(cur_rightServo - (degrees / 2));

  servoLeft.write(cur_leftServo - (degrees / 2));
  servoRight.write(cur_rightServo - (degrees / 2));

  cur_leftServo -= (degrees / 2);
  cur_rightServo -= (degrees / 2);
  actualStepValue = abs(int(y));

  if (actualStepValue > right_max)
  {
    right_max = actualStepValue;
    saveMovementData();
  }
  yield();
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

  auto updateMovementData = [&](const char *direction, int maxValue)
  {
    JsonArray arr = maxData.createNestedArray(direction);
    bool updated = false;
    for (int i = 0; i < doc[direction].size(); i++)
    {
      JsonObject obj = doc[direction][i];
      String date = obj["date"].as<String>();
      int data = obj["data"].as<int>();
      if (date == dateString)
      {
        if (maxValue > data)
        {
          obj["data"] = maxValue;
          updated = true;
        }
      }
      arr.add(obj);
    }
    if (!updated)
    {
      JsonObject newObj = arr.createNestedObject();
      newObj["date"] = dateString;
      newObj["data"] = maxValue;
    }
  };

  if (right_max > 0)
    updateMovementData("right", right_max);
  if (left_max > 0)
    updateMovementData("left", left_max);
  if (down_max > 0)
    updateMovementData("down", down_max);
  if (up_max > 0)
    updateMovementData("up", up_max);

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
              for (int i = 0; i < paramsNr; i++)
              {
                AsyncWebParameter *p = request->getParam(i);
                if (p->name() == "degrees")
                {
                  angleString = p->value();
                }
                if (p->name() == "repetitions")
                {
                  repetitionString = p->value();
                }
              }
              int degrees = angleString.toInt();
              int repetitions = repetitionString.toInt();
              request->send(200, "text/plain", "OK");
              move(degrees, repetitions, moveLeft); // Move by degrees
            });

  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Up handler");
              int paramsNr = request->params();
              String angleString;
              String repetitionString;
              for (int i = 0; i < paramsNr; i++)
              {
                AsyncWebParameter *p = request->getParam(i);
                if (p->name() == "degrees")
                {
                  angleString = p->value();
                }
                if (p->name() == "repetitions")
                {
                  repetitionString = p->value();
                }
              }
              int degrees = angleString.toInt();
              int repetitions = repetitionString.toInt();
              request->send(200, "text/plain", "OK");
              move(degrees, repetitions, moveUp); // Move by degrees
            });

  server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Right handler");
              int paramsNr = request->params();
              String angleString;
              String repetitionString;
              for (int i = 0; i < paramsNr; i++)
              {
                AsyncWebParameter *p = request->getParam(i);
                if (p->name() == "degrees")
                {
                  angleString = p->value();
                }
                if (p->name() == "repetitions")
                {
                  repetitionString = p->value();
                }
              }
              int degrees = angleString.toInt();
              int repetitions = repetitionString.toInt();
              request->send(200, "text/plain", "OK");
              move(degrees, repetitions, moveRight); // Move by degrees
            });

  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Down handler");
              int paramsNr = request->params();
              String angleString;
              String repetitionString;
              for (int i = 0; i < paramsNr; i++)
              {
                AsyncWebParameter *p = request->getParam(i);
                if (p->name() == "degrees")
                {
                  angleString = p->value();
                }
                if (p->name() == "repetitions")
                {
                  repetitionString = p->value();
                }
              }
              int degrees = angleString.toInt();
              int repetitions = repetitionString.toInt();
              request->send(200, "text/plain", "OK");
              move(degrees, repetitions, moveDown); // Replace 10 with the desired degree value
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
    {
      Serial.println("No significant movement detected, centering motors.");
      centerMotors();
    }
    lastTime = millis();
  }
}
