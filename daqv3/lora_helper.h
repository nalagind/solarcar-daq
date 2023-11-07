#include <RadioLib.h>

extern SX1262 radio;

void lora_init() {
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  radio.setRfSwitchPins(PA1, PA0);
  radio.setFrequency(915.0);
  radio.setBandwidth(500);
  radio.setSpreadingFactor(6);
  radio.setCodingRate(5);
  radio.setCRC(0);
  state = radio.setOutputPower(22);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
    while (true);
  }
}