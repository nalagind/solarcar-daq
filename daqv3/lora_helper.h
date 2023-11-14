#include <RadioLib.h>

extern SX1262 radio;

void lora_init(float freq, uint16_t bw, uint8_t sf, uint8_t cr, uint8_t crc) {
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  radio.setRfSwitchPins(PA1, PA0);
  radio.setFrequency(freq);
  radio.setBandwidth(bw);
  radio.setSpreadingFactor(sf);
  radio.setCodingRate(cr);
  radio.setCRC(crc);
  state = radio.setOutputPower(22);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("success!");
  } else {
    Serial.print("failed, code ");
    Serial.println(state);
    while (true);
  }
}

void LoRaTransmit(String str) {
  Serial.print(F("[SX1262] Transmitting packet ... "));
  digitalWrite(PA10, LOW);

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  int state = radio.transmit(str);

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F("success!"));

    // print measured data rate
    Serial.print(F("[SX1262] Datarate:\t"));
    Serial.print(radio.getDataRate()/1000);
    Serial.println(F(" kbps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }
}