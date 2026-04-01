#define BLYNK_TEMPLATE_ID   "TMPL3X7KBkWTw"
#define BLYNK_TEMPLATE_NAME "Blink"
#define BLYNK_AUTH_TOKEN    "ikXViE9NcBER3RAsDs1XlczNj7x0OywI"

#include <BlynkSimpleStream.h>

const int ledPin = 13;  // Built-in LED

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  Blynk.begin(Serial, BLYNK_AUTH_TOKEN);
}

// This function is called whenever the button on the app changes
BLYNK_WRITE(D13) {           // D13 matches the datastream you set in the app
  int pinValue = param.asInt();  // 0 = OFF, 1 = ON
  digitalWrite(ledPin, pinValue);
}

void loop() {
  Blynk.run();
}
