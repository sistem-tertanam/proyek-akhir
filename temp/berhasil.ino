uint16_t arduinoCounter = 0;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Arduino Ready.");
}

void loop() {
  // Jika ada data dari XMEGA
  if (Serial.available() > 0) {
    // Baca sampai '\n' (atau timeout default)
    String data = Serial.readStringUntil('\n');

    // Tampilkan di Serial Monitor
    Serial.print("Arduino received: ");
    Serial.println(data);

    // Balas => "Arduino: {counter}"
    Serial.print("Arduino: ");
    Serial.println(arduinoCounter++);
  }

  // Tugas lain...
  delay(10);
}
