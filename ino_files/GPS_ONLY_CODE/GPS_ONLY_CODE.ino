#include <SoftwareSerial.h>
#include <TinyGPS++.h>

SoftwareSerial gpsSerial(8,9);   // GPS: RX=8 (connect to GPS TX), TX=9 (connect to GPS RX)
TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600);
  Serial.println("GPS check started...");
}

void loop() {
  // Feed data to TinyGPS++
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Every 2 seconds, show status
  static unsigned long last = 0;
  if (millis() - last > 2000) {      // <-- changed from 1000 to 2000 (2 seconds)
    last = millis();

    if (gps.location.isValid() && gps.satellites.value() > 0) {
  // Just send lat,lon in one line
  Serial.print(gps.location.lat(), 6);
  Serial.print(",");
  Serial.println(gps.location.lng(), 6);
} else {
  // Simple "NOFIX" for Python to detect
  Serial.println("NOFIX");
}

}
}
