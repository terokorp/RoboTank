#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_PWMServoDriver.h>
#include <LittleFS.h>

// =========================
// WiFi Access Point
// =========================

const char *AP_SSID = "RoboTank";
const char *AP_PASSWORD = "12345678";

// =========================
// PCA9685
// =========================

// D4: Output Enable pin for PCA9685
#define OE_PIN D5
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVO_MIN 102 // ~500 us at 50 Hz
#define SERVO_MAX 512 // ~2500 us at 50 Hz

// =========================
// Web server
// =========================

ESP8266WebServer server(80);

// Current servo positions
int servo0 = 90;
int servo1 = 90;

float maxSpeed = 20.0f; // Maximum speed percentage (0-100)

// Convert angle to PCA9685 value
uint16_t angleToPWM(int angle)
{
  angle = constrain(angle, 0, 180);

  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

// Set servo angle
void setServo(uint8_t channel, int angle)
{
  digitalWrite(LED_BUILTIN, HIGH);
  angle = constrain(angle, 0, 180);
  Serial.println("Setting servo " + String(channel) + " to angle " + String(angle));
  pwm.setPWM(channel, 0, angleToPWM(angle));
  digitalWrite(LED_BUILTIN, LOW);
}


// =========================
// HTTP handlers
// =========================

void handleRoot()
{
    File file = LittleFS.open("/www/index.html", "r");

    if (!file) {
        server.send(404, "text/plain", "index.html not found");
        return;
    }

    server.streamFile(file, "text/html");
    file.close();
}

void handleServo()
{
    if (!server.hasArg("speed") || !server.hasArg("direction"))
    {
        server.send(400, "text/plain", "Missing parameters");
        return;
    }

    float speed = server.arg("speed").toFloat();
    float direction = server.arg("direction").toFloat();

    // Limit input values
    speed = constrain(speed, 0.0f, 100.0f);
    direction = constrain(direction, -180.0f, 180.0f);


    // Convert direction to radians
    float radians = direction * PI / 180.0f;

    // Convert joystick direction into X/Y movement
    //
    // 0°   = forward
    // +90° = right
    // -90° = left
    // 180° = backward
    //
    float forward = cos(radians) * speed * (maxSpeed / 100.0f);
    float turn    = sin(radians) * speed * (maxSpeed / 100.0f);


    // Differential/tank drive mixing
    float leftMotor  = forward + turn;
    float rightMotor = forward - turn;


    // Normalize if either motor exceeds 100
    float maxMotor = max(abs(leftMotor), abs(rightMotor));

    if (maxMotor > 100.0f)
    {
        leftMotor  = leftMotor  / maxMotor * 100.0f;
        rightMotor = rightMotor / maxMotor * 100.0f;
    }

    // Convert motor speed to continuous servo position
    //
    // 90 = stopped
    // >90 = one direction
    // <90 = opposite direction
    //
    int leftServo  = map((int)leftMotor,  -100, 100, 180, 0);
    int rightServo = map((int)rightMotor, -100, 100, 180, 0);

    // Save values
    servo0 = leftServo;
    servo1 = rightServo;

    // Output to PCA9685
    setServo(0, leftServo);
    setServo(1, rightServo);

    server.send(200, "text/plain", "OK");
}

void listFiles()
{
    Dir dir = LittleFS.openDir("/www/");

    while (dir.next())
    {
        Serial.print("FILE: ");
        Serial.print(dir.fileName());
        Serial.print("  SIZE: ");
        Serial.println(dir.fileSize());
    }
}
// =========================
// Setup
// =========================

void setup()
{
  Serial.begin(115200);

  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  pinMode(LED_BUILTIN, OUTPUT);

  // I2C
  // ESP8266:
  // SDA = GPIO4 / D2
  // SCL = GPIO5 / D1
  Wire.begin(D2, D1);

  // PCA9685
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);

  delay(10);

  // Initial servo positions
  setServo(0, servo0);
  setServo(1, servo1);

  // Start WiFi Access Point
  WiFi.mode(WIFI_AP);

  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("WiFi AP started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Mount LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted");
  listFiles();

  // Web server
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.serveStatic("/", LittleFS, "/www/");

  server.begin();

  Serial.println("Web server started");
}

// =========================
// Loop
// =========================

void loop()
{
  server.handleClient();
}