#include <RadioLib.h>

extern SX1262 radio;

void FSK_init() {
  // initialize SX1262 FSK modem with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.beginFSK();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  state = radio.setFrequency(433.5);
  state = radio.setBitRate(250.0);
  state = radio.setFrequencyDeviation(10.0);
  state = radio.setRxBandwidth(395.0);
  state = radio.setOutputPower(10.0);
  //state = radio.setDataShaping(RADIOLIB_SHAPING_1_0);
  uint8_t syncWord[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
  state = radio.setSyncWord(syncWord, 8);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    while (true);
  }
  state = radio.setCRC(0);
}

void FSK_Transmit(String str) {
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

void FSK_Receive(String str) {
	state = radio.receive(str);
  /*
    byte byteArr[8];
    int state = radio.receive(byteArr, 8);
  */
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Received packet!"));
    Serial.print(F("[SX1262] Data:\t"));
    Serial.println(str);
	/*      // print RSSI (Received Signal Strength Indicator)
      Serial1.print(F("[SX1262] RSSI:\t\t"));
      Serial1.print(radio.getRSSI());
      Serial1.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial1.print(F("[SX1262] SNR:\t\t"));
      Serial1.print(radio.getSNR());
      Serial1.println(F(" dB"));

      // print frequency error
      Serial1.print(F("[SX1262] Frequency error:\t"));
      Serial1.print(radio.getFrequencyError());
      Serial1.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
	Serial1.println(F("CRC error!"));}*/

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println(F("[SX1262] Timed out while waiting for packet!"));
  } else {
    Serial.println(F("[SX1262] Failed to receive packet, code "));
    Serial.println(state);
  }
}