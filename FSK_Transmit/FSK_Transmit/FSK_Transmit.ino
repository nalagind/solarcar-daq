#include <RadioLib.h>
#include <SPI.h>

HardwareSerial Serial1(PC5, PB10);
SPIClass SPI_3(PC12, PC11, PC10);
SPISettings spiSettings(14000000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);



void setup() {
  Serial1.begin(115200);
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
  radio.setRfSwitchPins(PA1, PA0);
  state = radio.setFrequency(915);
  state = radio.setBitRate(250.0);
  state = radio.setFrequencyDeviation(10.0);
  state = radio.setRxBandwidth(500);
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
int count = 0;
void loop(){

Serial1.print(F("[SX1262] Transmitting packet ... "));
  digitalWrite(PA10, LOW);

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  String str = "Hello World! #" + String(count++);
  int state = radio.transmit(str);

  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial1.println(F("success!"));

    // print measured data rate
    Serial1.print(F("[SX1262] Datarate:\t"));
    Serial1.print(radio.getDataRate());
    Serial1.println(F(" bps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial1.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occured while transmitting packet
    Serial1.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial1.print(F("failed, code "));
    Serial1.println(state);

  }

  // wait for a second before transmitting again
  digitalWrite(PA10, HIGH);
  delay(1000);

}