# KODE ALIF
#include <SoftwareSerial.h>
#define LED_PIN 7 

SoftwareSerial espSerial(5, 6); // RX, TX
uint16_t arduinoCounter = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); 
  
  Serial.begin(9600);
  while (!Serial) {
    ; 
  }
  Serial1.begin(9600);
  Serial.println("Ready to receive data");
  Serial2.begin(9600);
  espSerial.begin(9600); // For communication with ESP8266
  delay(2000);
}

void loop() {
  delay(100);

  if (Serial.available()) {
    Serial.println("masuk");
    String incomingData = Serial.readStringUntil('\n');
    incomingData.trim(); 

    Serial.println("Received: " + incomingData);

    // if (incomingData == "coming from ESP8266: 1") {
       if (incomingData.indexOf("XMEGA") == -1) {
        incomingData += '\n';
        Serial.println("LED is ON");
        
        char buf[64];
        int timeout = 0;
        int idx = 0;
        // Tampilkan di Serial Monitor
        
        // Check the first character of incomingData
        if (incomingData[0] == '1') {
            // Write to Serial1
            int timeout = 0;
            int idx = 2;
            while (idx < incomingData.length() && timeout < 256) {
                if (Serial1.availableForWrite() > 0) {
                    Serial1.write(incomingData[idx]);
                    idx++;
                    delay(1200);
                }
                timeout++;
            }
        } else if (incomingData[0] == '2') {
            // Write to Serial2
            int timeout = 0;
            int idx = 2;
            while (idx < incomingData.length() && timeout < 256) {
                if (Serial2.availableForWrite() > 0) {
                    Serial2.write(incomingData[idx]);
                    idx++;
                    delay(1200);
                }
                timeout++;
            }
        } else {
            Serial.println("Invalid destination in incomingData[0]");
        }
        
        arduinoCounter++;
    } 
    else if (incomingData == "coming from ESP8266: 0") {
//      digitalWrite(LED_PIN, LOW); // Turn off the LED
//      Serial.println("LED is OFF");
    }
  }
}

void serialEvent1() {
  Serial.println("masuk serial 1");
  String inp = "";
  bool complete = false;
  while (Serial1.available()) {
    char inChar = (char)Serial1.read();
    inp += inChar;
    if (inChar == '\n') {
      complete = true;
    }
  }
  Serial.println("Received: " + inp);
  espSerial.println("Received: " + inp);
}

void serialEvent2() {
  Serial.println("masuk serial 1");
  String inp = "";
  bool complete = false;
  while (Serial2.available()) {
    char inChar = (char)Serial2.read();
    inp += inChar;
    if (inChar == '\n') {
      complete = true;
    }
  }
  Serial.println("Received: " + inp);
  espSerial.println("Received: " + inp);
  
}