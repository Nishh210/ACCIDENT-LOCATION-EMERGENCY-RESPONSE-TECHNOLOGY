#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ================== MODULE PINS ==================
const int MPU_addr = 0x68;          // MPU6050 I2C address
SoftwareSerial BT(2, 3);            // HC-05 Bluetooth: RX, TX
SoftwareSerial gpsSerial(8, 9);     // GPS: RX, TX
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
const float PITCH_THRESHOLD_MIN = 40.0;  // Lower safe limit
const float PITCH_THRESHOLD_MAX = 330.0; // Upper safe limit

// ================== SETUP ==================
void setup() {
    Wire.begin();
    BT.begin(38400);
    gpsSerial.begin(9600);

    pinMode(buttonPin, INPUT_PULLUP);  // Button connected to GND

    // Wake up MPU6050
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);   // PWR_MGMT_1 register
    Wire.write(0);      // Wake up
    Wire.endTransmission(true);

    BT.println("Calibrating gyro bias...");
    calibrateGyro();
    lastTime = millis();
    BT.println("Pitch tracking started. Initial pitch set to 0°");
}

void loop() {
    // --- Check push button ---
    if (digitalRead(buttonPin) == LOW) { // Button pressed (connected to GND)
        pitch = 0;                       // Reset pitch
        BT.println("Pitch set to zero"); // Send message via Bluetooth
        Serial.println("Pitch set to zero"); // Optional: print to Serial
        delay(500); // Debounce delay
    }

    // --- Read gyro X ---
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x43);  // Gyro X register
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
        String message = "ALERT! Unsafe tilt detected.\n";
        message += "Pitch: " + String(pitch, 2) + "°\n";
        if (gps.location.isValid()) {
            message += "Location: " + String(latitude, 6) + ", " + String(longitude, 6);
        } else {
            message += "Location: Not Available";
        }

        BT.println(message);
        Serial.println(message);
        delay(5000); // avoid spamming alerts
    }

    delay(200);
}

// ================== GYRO CALIBRATION ==================
void calibrateGyro() {
    long sum = 0;
    for (int i = 0; i < calibrationSamples; i++) {
        Wire.beginTransmission(MPU_addr);
        Wire.write(0x43);  // Gyro X register
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_addr, 2, true);
        int16_t rawGyX = Wire.read() << 8 | Wire.read();
        sum += rawGyX;
        delay(2);
    }
    gyroBiasX = (float)sum / calibrationSamples;
    BT.print("Gyro X bias: ");
    BT.println(gyroBiasX);
}
