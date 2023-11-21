#include <RadioLib.h>
#include <SPI.h>

HardwareSerial Serial1(PC5, PB10);
SPIClass SPI_3(PC12, PC11, PC10);
SPISettings spiSettings(14000000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);



void setup() {
  Serial1.begin(115200);
  pinMode(PA9, OUTPUT);
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

  radio.setPacketReceivedAction(setFlag);
  Serial1.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}
void loop(){
if(receivedFlag) {
    digitalWrite(PA9, LOW);
    // reset flag
    receivedFlag = false;
    String str;
    int state = radio.readData(str);
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