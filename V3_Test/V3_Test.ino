#include <STM32_CAN.h>
#include <SD.h>
#include <SPI.h>
#include <STM32RTC.h>
#include <RadioLib.h>

SPIClass SPI_3(PC12, PC11, PC10);
SPISettings spiSettings(14000000, MSBFIRST, SPI_MODE0);

// Initialize SX1262 LoRa module
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);
STM32_CAN Can( CAN1, ALT ); 
HardwareSerial Serial1(PC5, PB10);

static CAN_message_t CAN_RX_msg;

const byte seconds = 0;
const byte minutes = 20;
const byte hours = 16;

const byte weekDay = 1;
const byte day = 24;
const byte month = 9;
const byte year = 23;


/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();
void initCAN() {

    // By default the LSI is selected as source
    rtc.begin(); // initialize RTC 24H format
	  rtc.setTime(hours, minutes, seconds);
    rtc.setDate(weekDay, day, month, year);
    //initializeMessageNameMap();
    Can.begin();
    Can.setBaudRate(250000);  // Set the desired baud rate
    //Can.setBaudRate(500000);  //500KBPS
    //Can.setBaudRate(1000000);  //1000KBPS
    Serial1.println("ok");
}
void initLoRa(){
  // initialize SX1262 with default settings
  Serial1.print(F("[SX1262] Initializing ... "));
  int state = radio.begin();
  radio.setRfSwitchPins(PA1, PA0);
  radio.setFrequency(915.0);
  radio.setBandwidth(500);
  radio.setSpreadingFactor(6);
  radio.setCodingRate(5);
  radio.setCRC(0);
  state = radio.setOutputPower(22);
  if (state == RADIOLIB_ERR_NONE) {
    Serial1.println(F("success!"));
  } else {
    Serial1.print(F("failed, code "));
    Serial1.println(state);
    while (true);
  }
}
String processReceivedMessage(const CAN_message_t& msg) {
  String output = "Timestamp: ";

   // Prepare the timestamp
    output += rtc.getYear();
    output += "/";
    output += rtc.getMonth();
    output += "/";
    output += rtc.getDay();
    output += " ";
    output += rtc.getHours();
    output += ":";
    output += rtc.getMinutes();
    output += ":";
    output += rtc.getSeconds();
    output += " | ";

    // Prepare message details
    output += "\nChannel:";
    output += CAN_RX_msg.bus;
    
    // Print the received message's name if it's in the map
    /*if (messageNameMap.count(CAN_RX_msg.id)) {
        output += "\nMessage Name: ";
        output += messageNameMap[CAN_RX_msg.id];
    }*/
    if (CAN_RX_msg.flags.extended == false) {
        output += " \nStandard ID: ";
    } else {
        output += " \nExtended ID: ";
    }
    output += CAN_RX_msg.id;
    output += "\nDLC: ";
    output += CAN_RX_msg.len;

    if (CAN_RX_msg.flags.remote == false) {
        output += "\nbuf: ";
        for (int i = 0; i < CAN_RX_msg.len; i++) {
            output += "0x";
            output += CAN_RX_msg.buf[i];
            if (i != (CAN_RX_msg.len - 1)) output += " ";
        }
    } else {
        output += "\nData: REMOTE REQUEST FRAME";
    }
	Serial1.println(output);
	return output;
}

void writeFile(const char *path, String message) {
    Serial1.printf("Writing file: %s\n", path);

    File file =SD.open(path, FILE_WRITE);
    if (!file) {
        Serial1.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial1.println("File written");
    } else {
        Serial1.println("Write failed");
    }
    file.close();
}

void createDir(const char *path) {
    Serial1.printf("Creating Dir: %s\n", path);
    if (SD.mkdir(path)) {
        Serial1.println("Dir created");
    } else {
        Serial1.println("mkdir failed");
    }
}
void LoRaTransmit(String str) {
  Serial1.print(F("[SX1262] Transmitting packet ... "));
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
    Serial1.println(F("success!"));

    // print measured data rate
    Serial1.print(F("[SX1262] Datarate:\t"));
    Serial1.print(radio.getDataRate()/1000);
    Serial1.println(F(" kbps"));

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
}
void setup() {
  // put your setup code here, to run once:
  Serial1.begin(115200);
  pinMode(PA9, OUTPUT);
  pinMode(PA10, OUTPUT);
  pinMode(PC4, OUTPUT);
  initLoRa();
  initCAN();
  SPIClass SPI_1(PA7,PA6,PA5);
  SPI_1.begin();
  if (!SD.begin(PC4)) {
      Serial1.println("no SD card attached");
      while (true);
    } else {
      Serial1.println("SD mounted");
    }
  createDir("test.txt");
  Serial1.println("ok");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Can.read(CAN_RX_msg) ) {
    digitalWrite(PA9, LOW);
    String output = processReceivedMessage(CAN_RX_msg);
    writeFile("test.txt", output);
    LoRaTransmit(output);
    digitalWrite(PA9, HIGH);
  }

}
