#define BLYNK_TEMPLATE_ID "TMPL6wtjOroeN"
#define BLYNK_TEMPLATE_NAME "Dashboard Smart Building"
#define BLYNK_AUTH_TOKEN "k1R02uJD22Gwsw-88Dh5AkKTPjF_JGP2"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "syauqi";
char pass[] = "12345678";

String str;

void setup() {
  Serial.begin(9600); // For debugging
  Serial1.begin(9600); // For communication with Arduino

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  Serial.println("ESP8266: Blynk Initialized");
}

// Handler for Blynk virtual button on V0
BLYNK_WRITE(V0) {
  int state = param.asInt(); // 0 or 1
  Serial.print("LED:");
  Serial.println(state);
  str =String("1-LED:")+String(state);
  Serial1.println(str);
}
BLYNK_WRITE(V5) {
  int state = param.asInt(); // 0 or 1
  Serial.print("W:");
  Serial.println(state);
  str =String("2-W:")+String(state);
  Serial1.println(str);
}
BLYNK_WRITE(V6) {
  int state = param.asInt(); // 0 or 1
  Serial.print("F:");
  Serial.println(state);
  str =String("2-F:")+String(state);
  Serial1.println(str);
}
BLYNK_WRITE(V7) {
  int state = param.asInt(); // 0 or 1
  Serial.print("B:");
  Serial.println(state);
  str =String("2-B:")+String(state);
  Serial1.println(str);
}


void loop() {
  Blynk.run(); // Run Blynk
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    Serial.println("Received Data: " + data);
    parseData(data);
  }
}

void parseData(String data) {
  // Split the input data into key-value pairs
  while (data.length() > 0) {
    int delimiterIndex = data.indexOf(' '); // Find the space delimiter
    String keyValue = (delimiterIndex != -1) ? data.substring(0, delimiterIndex) : data; // Get the key-value pair

    int separatorIndex = keyValue.indexOf(':'); // Find the colon separator
    if (separatorIndex != -1) {
      String key = keyValue.substring(0, separatorIndex); // Extract the key
      int value = keyValue.substring(separatorIndex + 1).toInt(); // Extract the value and convert to int

      // Map keys to Blynk virtual pins
      if (key == "LED") Blynk.virtualWrite(V0, value);
      else if (key == "PIR") Blynk.virtualWrite(V1, value);
      else if (key == "L") Blynk.virtualWrite(V2, value);
      else if (key == "T") Blynk.virtualWrite(V3, value);
      else if (key == "H") Blynk.virtualWrite(V4, value);
      else if (key == "Q") Blynk.virtualWrite(V5, value);
      else if (key == "W") Blynk.virtualWrite(V6, value);
      else if (key == "F") Blynk.virtualWrite(V7, value);
      else if (key == "B") Blynk.virtualWrite(V7, value); // Add more mappings if needed

      // Debug output
      Serial.println("Key: " + key + " | Value: " + String(value));
    }

    // Remove the processed key-value pair
    data = (delimiterIndex != -1) ? data.substring(delimiterIndex + 1) : "";
  }
}