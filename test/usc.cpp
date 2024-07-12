#include <Arduino.h>
#include "usc.h"
#include "Wire.h"

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

TwoWire Wire2(uscL.p(I2C_SDA), uscL.p(I2C_SCL));
TwoWire Wire3(uscC.p(I2C_SDA), uscC.p(I2C_SCL));

void setup() {
    Serial.setRx(uscR.p(UART_RX));
    Serial.setTx(uscR.p(UART_TX));
    Serial.begin(115200);
    Serial.println("ok");

    // pinMode(uscR.p(I2C_SDA), INPUT_PULLUP);
    // pinMode(uscR.p(I2C_SCL), INPUT_PULLUP);
    // digitalWrite(uscR.p((I2C_SDA)), HIGH);
    // digitalWrite(uscR.p(I2C_SCL), HIGH);

    Wire.begin(uscR.p(I2C_SDA), uscR.p(I2C_SCL));
    Wire2.begin();
    Wire3.begin();
}

void loop() {
    Wire.beginTransmission(0x68);
    Wire.write(117);
    Wire.endTransmission();
    Wire.requestFrom(0x68, 1);
    
    Wire2.beginTransmission(0x68);
    Wire2.write(117);
    Wire2.endTransmission();
    Wire2.requestFrom(0x68, 1);
    
    Wire3.beginTransmission(0x68);
    Wire3.write(117);
    Wire3.endTransmission();
    Wire3.requestFrom(0x68, 1);

    // digitalToggle(uscR.p(I2C_SDA));
    // digitalToggle(uscR.p(I2C_SCL));
    delay(500);

}
