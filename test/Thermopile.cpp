#include <Arduino.h>

#include <Wire.h>
#include "SparkFunMLX90614.h"
#include "usc.h"

IRTherm therm;
float temperature;

USC usc = uscL;

const PinMap PinMap_I2C_SDA[] = {
    {PB_7,  I2C1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C1)},
    {PB_9,  I2C2, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF9_I2C2)},
    {PC_9,  I2C3, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C3)}
};

const PinMap PinMap_I2C_SCL[] = {
    {PB_8,  I2C1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C1)},
    {PB_10, I2C2, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C2)},
    {PA_8,  I2C3, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C3)}
};

//                     RX    TX
HardwareSerial Serial1(usc.p(UART_RX), usc.p(UART_TX));
void setup() {
  delay(1000);
  Serial1.begin(115200);
  Wire.setSDA(usc.p(I2C_SDA));
  Wire.setSCL(usc.p(I2C_SCL));
  Wire.begin();

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

    Serial1.print(temperature);
    Serial1.println("C");
  }

  delay(1000);
}