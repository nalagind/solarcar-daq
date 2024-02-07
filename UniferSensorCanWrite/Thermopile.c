#include <Wire.h>
#include <SparkFunMLX90614.h>
#include "STM32_CAN.h"

IRTherm therm;
float temperature;
static CAN_message_t CAN_TX_msg;

STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.

//                     RX    TX
HardwareSerial Serial1(PA3, PA2);
void setup() {
  delay(1000);
  Serial1.begin(115200);
  Wire.setSDA(PC9);
  Wire.setSCL(PA8);
  Wire.begin();
  Can.begin();
  Can.setBaudRate(500000);

  if (therm.begin() == false){ // Initialize thermal IR sensor
    Serial1.println("Fuck you");
    while(1);
  }
  Serial1.println("Thermopile Acknowlwdge");
  therm.setUnit(TEMP_C);
}

void loop() {
  if (therm.read()) { // On success, read() will return 1, on fail 0.
  // Use the object() and ambient() functions to grab the object and ambient
	// temperatures.
	// They'll be floats, calculated out to the unit you set with setUnit().
    temperature = therm.object();

    CAN_TX_msg.id = (0xF11);
    CAN_TX_msg.len = 4;
    CAN_TX_msg.buf[0] = temperature;
    Can.write(CAN_TX_msg);

    Serial1.print(temperature);
    Serial1.println("C");
  }

  delay(1000);
}