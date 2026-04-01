#include <Wire.h>

const int MPU_addr = 0x68;

float pitch = 0.0;
float gyroBiasX = 0.0;
int calibrationSamples = 500;

unsigned long lastTime = 0;
const int buttonPin = 4;  // Button to GND

void setup() {
    Wire.begin();
    Serial.begin(9600);

    pinMode(buttonPin, INPUT_PULLUP);

    // Wake up MPU6050
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    Serial.println("Calibrating gyro bias...");
    calibrateGyro();
    lastTime = millis();
    Serial.println("Pitch tracking started. Initial pitch = 0°");
}

void loop() {

  // Reset pitch using button
  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Button pressed → Pitch reset to 0");
    pitch = 0;
    delay(500);
  }

  // Read Gyro X
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 2, true);
  int16_t rawGyX = Wire.read() << 8 | Wire.read();

  // Time delta
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  // Convert to deg/sec
  float gyroX_deg = ((float)rawGyX - gyroBiasX) / 131.0;

  // Integrate → pitch
  pitch += gyroX_deg * dt;

  // Wrap 0–360°
  if (pitch < 0) pitch += 360;
  if (pitch >= 360) pitch -= 360;

  // Print to Serial Monitor
  Serial.print("Pitch: ");
  Serial.println(pitch);

  // Alert condition
  if (pitch >= 40 && pitch <= 330) {
    Serial.println("⚠️ ALERT (Tilt detected)");
  }

  delay(500);
}

// ===== CALIBRATION =====
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

    Serial.print("Gyro X bias: ");
    Serial.println(gyroBiasX);
}