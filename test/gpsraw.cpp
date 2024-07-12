#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "dmaserial.h"

TinyGPSPlus gps;

DmaSerial ss;

void setup() {
    Serial.setRx(PC5);
    Serial.setTx(PB10);
    Serial.begin(115200);

    ss.begin(9600);
}

void loop() {
    while (ss.available()) {
        Serial.write(ss.read());
    }

}