/*
   RadioLib SX126x Receive with Interrupts Example

   This example listens for LoRa transmissions and tries to
   receive them. Once a packet is received, an interrupt is
   triggered. To successfully receive data, the following
   settings have to be the same on both transmitter
   and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word

   Other modules from SX126x family can also be used.

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
  pinMode(PA9, OUTPUT);

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

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial1.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial1.println(F("success!"));
  } else {
    Serial1.print(F("failed, code "));
    Serial1.println(state);
    while (true);
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.scanChannel();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void loop() {
  // check if the flag is set
  if(receivedFlag) {
    digitalWrite(PA9, LOW);
    // reset flag
    receivedFlag = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int state = radio.readData(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial1.println(F("[SX1262] Received packet!"));

      // print data of the packet
      Serial1.print(F("[SX1262] Data:\t\t"));
      Serial1.println(str);

      // print RSSI (Received Signal Strength Indicator)
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
      Serial1.println(F("CRC error!"));

    } else {
      // some other error occurred
      Serial1.print(F("failed, code "));
      Serial1.println(state);

    }
    digitalWrite(PA9, HIGH);
  }
}
