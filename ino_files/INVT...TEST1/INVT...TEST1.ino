#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ================== MODULE PINS ==================
const int MPU_addr = 0x68;          // MPU6050 I2C address
SoftwareSerial BT(2, 3);            // RX, TX (Arduino:2 → HC-05 TX, 3 → HC-05 RX with divider)
SoftwareSerial gpsSerial(8, 9);     // GPS: RX=8 (connect to GPS TX), TX=9 (connect to GPS RX)
const int buttonPin = 4;            // Button to reset pitch

// ================== MPU6050 VARIABLES ==================
float pitch = 0.0;
float gyroBiasX = 0.0;
int calibrationSamples = 500;
unsigned long lastTime = 0;

// ================== GPS VARIABLES ==================
TinyGPSPlus gps;
double latitude = 0.0, longitude = 0.0;

// ================== SAFETY THRESHOLDS ==================
const float PITCH_THRESHOLD_MIN = 30.0;  // Lower safe limit
const float PITCH_THRESHOLD_MAX = 310.0; // Upper safe limit

// ================== SETUP ==================
void setup() {
    Wire.begin();
    BT.begin(38400);
    gpsSerial.begin(9600);
    Serial.begin(9600);  // Added for Serial Monitor output

    pinMode(buttonPin, INPUT_PULLUP);  // Button connected to GND

    // Wake up MPU6050
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);   // PWR_MGMT_1 register
    Wire.write(0);      // Wake up
    Wire.endTransmission(true);

    Serial.println("Calibrating gyro bias...");
    calibrateGyro();
    lastTime = millis();
    Serial.println("Pitch tracking started. Initial pitch set to 0°");
}

// ================== LOOP ==================
void loop() {
    // --- Check push button ---
    if (digitalRead(buttonPin) == LOW) {
        pitch = 0;
        Serial.println("Pitch set to zero");
        BT.println("Pitch set to zero");
        delay(500);
    }

    // --- Read gyro X ---
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 2, true);
    int16_t rawGyX = Wire.read() << 8 | Wire.read();

    // --- Time delta ---
    unsigned long currentTime = millis();
    float dt = (currentTime - lastTime) / 1000.0;
    lastTime = currentTime;

    // --- Convert gyro to deg/s ---
    float gyroX_deg = ((float)rawGyX - gyroBiasX) / 131.0;

    // --- Integrate pitch ---
    pitch += gyroX_deg * dt;

    // --- Wrap pitch to 0–360° ---
    if (pitch < 0) pitch += 360;
    if (pitch >= 360) pitch -= 360;

    // --- Read GPS data ---
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
        if (gps.location.isUpdated()) {
            latitude = gps.location.lat();
            longitude = gps.location.lng();
        }
    }

    // --- Check for unsafe pitch range ---
    if (pitch >= PITCH_THRESHOLD_MIN && pitch <= PITCH_THRESHOLD_MAX) {
        String message = "ALERT!\n";
        message += "Pitch: " + String(pitch, 2) + "°\n";
        if (gps.location.isValid()) {
            message += "Location: " + String(latitude, 6) + ", " + String(longitude, 6);
        } else {
            message += "Location: Not Available";
        }
        BT.println(message);
        Serial.println(message);
        delay(1000);
    }

    delay(200);
}

// ================== GYRO CALIBRATION ==================
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