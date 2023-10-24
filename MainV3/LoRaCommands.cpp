#include "LoRaCommands.h"
#include <SPI.h>


SPIClass SPI_3(PC12, PC11, PC10);
SPISettings spiSettings(14000000, MSBFIRST, SPI_MODE0);

// Initialize SX1262 LoRa module
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);


// ... Define other parameters and variables ...

void setupLoRa() {
  // initialize SX1262 with default settings
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
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
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

  // wait for a second before transmitting again
  digitalWrite(PA10, HIGH);
}


// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!

void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}
String LoRaReceive() {
  //   // check if the flag is set
  // if(receivedFlag) {
  //   digitalWrite(PA9, LOW);
  //   // reset flag
  //   receivedFlag = false;

  //   // you can read received data as an Arduino String
  //   String str;
  //   int state = radio.readData(str);
	// String output = "";
	
  //   // you can also read received data as byte array
  //   /*
  //     byte byteArr[8];
  //     int state = radio.readData(byteArr, 8);
  //   */

  //   if (state == RADIOLIB_ERR_NONE) {
  //     // packet was successfully received
  //     Serial.println(F("[SX1262] Received packet!"));
	//   String decodedData = decodeData(str)
  //     // print data of the packet
  //     Serial.print(F("[SX1262] Data:\t\t"));
  //     Serial.println(decodedData);
	// 	output += decodedData + "\n";
  //     // print RSSI (Received Signal Strength Indicator)
  //     Serial.print(F("[SX1262] RSSI:\t\t"));
  //     Serial.print(radio.getRSSI());
  //     Serial.println(F(" dBm"));
	// output += radio.getRSSI() + " dBm" + "\n";
	
  //     // print SNR (Signal-to-Noise Ratio)
  //     Serial.print(F("[SX1262] SNR:\t\t"));
  //     Serial.print(radio.getSNR());
  //     Serial.println(F(" dB"));
	  
	//   output += radio.getSNR() + " dB" + "\n";

  //     // print frequency error
  //     Serial.print(F("[SX1262] Frequency error:\t"));
  //     Serial.print(radio.getFrequencyError());
  //     Serial.println(F(" Hz"));
	  
	//   output += radio.getFrequencyError() + " Hz" + "\n";

  //   } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
  //     // packet was received, but is malformed
  //     Serial.println(F("CRC error!"));

  //   } else {
  //     // some other error occurred
  //     Serial.print(F("failed, code "));
  //     Serial.println(state);

  //   }
  //   digitalWrite(PA9, HIGH);
	// return output;
  // }
}

// Delta compression function
void deltaEncode(String* data, int length, String* compressedData) {
 compressedData[0] = data[0];  // First string remains unchanged
    for (int i = 1; i < length; i++) {
        String diff;
        for (int j = 0; j < data[i].length(); j++) {
            int diffValue = data[i][j] - data[i - 1][j];
            diff += String(diffValue);
            if (j != data[i].length() - 1) {
                diff += ",";
            }
        }
        compressedData[i] = diff;
    }
}

void deltaDecode(String* compressedData, int length, String* decodedData) {
    decodedData[0] = compressedData[0];  // First value remains unchanged
    for (int i = 1; i < length; i++) {
        String decodedValue = decodedData[i - 1];
        String deltaValues = compressedData[i];
        int start = 0;
        int end = deltaValues.indexOf(",");
        
        for (int j = 0; j < decodedValue.length(); j++) {
            int delta = deltaValues.substring(start, end).toInt();
            decodedValue[j] = char(decodedValue[j] + delta);
            
            start = end + 1;
            end = deltaValues.indexOf(",", start);
            if (end == -1) {
                end = deltaValues.length();
            }
        }
        
        decodedData[i] = decodedValue;
    }
}