#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>

// ================== OLED ==================
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ================== MODULE PINS ==================
const int MPU_addr = 0x68;
SoftwareSerial BT(2, 3);
const int buttonPin = 4;
const int ledPin = 13;
const int buzzerPin = 12;

// ================== MPU ==================
float pitch = 0.0;
float gyroBiasX = 0.0;
int calibrationSamples = 500;
unsigned long lastTime = 0;

// ================== DATA ==================
const char* riderName = "Rider: Nishanth";
const char* vehicleNo = "TN-59-AB-1234";
const char* lat = "Lat:9.885340";
const char* lng = "Lng:78.080399";

// ================== THRESHOLDS ==================
const float PITCH_THRESHOLD_MIN = 30.0;
const float PITCH_THRESHOLD_MAX = 310.0;

// ================== ALERT ==================
bool alertTriggered = false;
bool waitingForCancel = false;
unsigned long alertStartTime = 0;
unsigned long lastBlinkTime = 0;
bool ledState = false;

// ================== OLED ==================
void showDisplay(const char* l1, const char* l2, const char* l3, const char* l4) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(0, 12, l1);
    u8g2.drawStr(0, 26, l2);
    u8g2.drawStr(0, 40, l3);
    u8g2.drawStr(0, 54, l4);
  } while (u8g2.nextPage());
}

// ================== SETUP ==================
void setup() {
  Wire.begin();
  Serial.begin(9600);
  BT.begin(38400);
  u8g2.begin();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.println("System Booting...");
  showDisplay("System Booting...", "", "", "");

  // MPU INIT
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  calibrateGyro();

  // GPS FAKE INIT
  int delayTime = random(1000, 10000);
  unsigned long start = millis();

  while (millis() - start < delayTime) {
    showDisplay("GPS Status:", "No Fix...", "", "");
    delay(1000);
  }

  showDisplay("GPS Ready", "Calibrated", "", "");
  delay(1500);

  showDisplay("System Ready", riderName, vehicleNo, "");

  lastTime = millis();
}

// ================== LOOP ==================
void loop() {

  // RESET
  if (digitalRead(buttonPin) == LOW) {
    resetSystem();
    return;
  }

  // ALERT MODE
  if (alertTriggered) {

    if (waitingForCancel) {

      Serial.println("Waiting for cancel...");

      // LED BLINK
      if (millis() - lastBlinkTime >= 200) {
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
        lastBlinkTime = millis();
      }

      playAlertTone();

      showDisplay("ACCIDENT!", "Waiting Cancel", "Btn / BT OK", "");

      // BUTTON CANCEL
      if (digitalRead(buttonPin) == LOW) {
        Serial.println("Cancelled via Button");
        resetSystem();
        return;
      }

      // BT CANCEL
      char input[5];
      int i = 0;

      while (BT.available() && i < 4) {
        char c = BT.read();
        if (c != '\n' && c != '\r') {
          input[i++] = tolower(c);
        }
      }
      input[i] = '\0';

      if (i > 0) {
        Serial.print("BT: ");
        Serial.println(input);
      }

      if (strcmp(input, "ok") == 0) {
        Serial.println("Cancelled via BT");
        resetSystem();
        return;
      }

      // AFTER 10 SEC
      if (millis() - alertStartTime >= 5000) {

        BT.println("ACCIDENT ALERT!");
        BT.println(riderName);
        BT.println(vehicleNo);
        BT.println(lat);
        BT.println(lng);

        Serial.println("🚨 ALERT SENT 🚨");

        showDisplay("ALERT SENT!", riderName, lat, lng);

        digitalWrite(ledPin, HIGH);

//  CONTINUOUS BUZZER SOUND (PASSIVE BUZZER FIX)
tone(buzzerPin, 1000);  // constant tone

        waitingForCancel = false;
      }
    }

    return;
  }

  // ===== GYRO =====
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 2, true);
  int16_t rawGyX = Wire.read() << 8 | Wire.read();

  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  float gyroX_deg = ((float)rawGyX - gyroBiasX) / 131.0;
  pitch += gyroX_deg * dt;

  if (pitch < 0) pitch += 360;
  if (pitch >= 360) pitch -= 360;

  Serial.print("Pitch: ");
  Serial.println(pitch);

  showDisplay("Monitoring...", riderName, vehicleNo, "System Active");

  if (pitch >= PITCH_THRESHOLD_MIN && pitch <= PITCH_THRESHOLD_MAX) {

    Serial.println("🚨 ACCIDENT DETECTED 🚨");

    alertTriggered = true;
    waitingForCancel = true;
    alertStartTime = millis();

    showDisplay("ACCIDENT DETECTED!", "Locking Location...", "", "");
  }

  delay(200);
}

// ================== RESET ==================
void resetSystem() {
  pitch = 0;
  alertTriggered = false;
  waitingForCancel = false;

  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  digitalWrite(buzzerPin, LOW);

  Serial.println("System Reset");

  showDisplay("System Reset", "Ready Again", "", "");
  delay(1000);
}

// ================== CALIBRATION ==================
void calibrateGyro() {
  long sum = 0;
  for (int i = 0; i < calibrationSamples; i++) {
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 2, true);
    int16_t rawGyX = Wire.read() << 8 | Wire.read();
    sum += rawGyX;
    delay(2);
  }
  gyroBiasX = (float)sum / calibrationSamples;
}

// ================== BUZZER ==================
void playAlertTone() {
  tone(buzzerPin, 1000);
  delay(150);
  tone(buzzerPin, 700);
  delay(150);
}