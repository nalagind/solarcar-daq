/*
  RadioLib SX126x Blocking Transmit Example

  This example transmits packets using SX1262 LoRa radio module.
  Each packet contains up to 256 bytes of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

  Other modules from SX126x family can also be used.

  Using blocking transmit is not recommended, as it will lead
  to inefficient use of processor time!
  Instead, interrupt transmit is recommended.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>
#include <SPI.h>

HardwareSerial Serial1(PC5, PB10);
SPIClass SPI_3(PC12, PC11, PC10);
SPISettings spiSettings(14000000, MSBFIRST, SPI_MODE0);

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1262 radio = RadioShield.ModuleA;

// or using CubeCell
//SX1262 radio = new Module(RADIOLIB_BUILTIN_MODULE);

void setup() {
  Serial1.begin(115200);
  pinMode(PA10, OUTPUT);

  // initialize SX1262 with default settings
  Serial1.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial1.println(F("success!"));
  } else {
    Serial1.print(F("failed, code "));
    Serial1.println(state);
    while (true);
  }

  // some modules have an external RF switch
  // controlled via two pins (RX enable, TX enable)
  // to enable automatic control of the switch,
  // call the following method
  // RX enable:   4
  // TX enable:   5
  
  radio.setRfSwitchPins(PA1, PA0);
  
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
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
