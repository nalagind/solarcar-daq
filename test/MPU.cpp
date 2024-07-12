#include <Arduino.h>
#include<Wire.h>
#include "usc.h"
int UNIF_LED = PC7;

const int MPU_addr=0x68;
int16_t AX_raw,AY_raw,AZ_raw;
int16_t AX,AY,AZ;
byte X_raw_high, X_raw_low, Y_raw_high, Y_raw_low, Z_raw_high, Z_raw_low;

USC usc = uscL;

//                      RX    TX
// HardwareSerial Serial1(PA3, PA2);
HardwareSerial Serial1(usc.p(UART_RX), usc.p(UART_TX));

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

int i = 0;
int j = 6;

void setup() {
  Serial1.begin(115200);
  Serial1.println("ok");
  pinMode(UNIF_LED, OUTPUT);

  // Wire.setSDA(PC9);
  // Wire.setSCL(PA8);
  // Wire.setSDA(PB9);
  // Wire.setSCL(PB10);
  Wire.setSDA(usc.p(I2C_SDA));
  Wire.setSCL(usc.p(I2C_SCL));
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x75); // WHO_AM_I register address
  // Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(MPU_addr, 1);
  byte r = Wire.read();
  Serial1.println(r, HEX);

  Wire.beginTransmission(MPU_addr);  // Device address.
  Wire.write(0x6B);              // PWR_MGMT_1 register.
  Wire.write(0b10001000);        // DEVICE_RESET, TEMP_DIS.
  Wire.endTransmission();
  delay(100);                    // Wait for reset to complete.

  Wire.beginTransmission(MPU_addr);  // Device address.
  Wire.write(0x68);              // SIGNAL_PATH_RESET register.
  Wire.write(0b111);        // GYRO_RESET, ACCEL_RESET, TEMP_RESET.
  Wire.endTransmission();
  delay(100);                    // Wait for reset to complete.

  // Disable SLEEP mode because the reset re-enables it. Section 3, PWR_MGMT_1 register, page 8.
  Wire.beginTransmission(MPU_addr);   // Device address.
  Wire.write(0x6B);                     // PWR_MGMT_1 register.
  Wire.write(0b00001000);               // SLEEP = 0, TEMP_DIS = 1.
  Wire.endTransmission();
}

void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); //0x3B is the accel address
  Wire.endTransmission();
  Wire.requestFrom(MPU_addr,j,true);
  i = j;


  while (Wire.available()) {
    // X_raw_high = Wire.read();
    // X_raw_low = Wire.read();
    // Y_raw_high = Wire.read();
    // Y_raw_low = Wire.read();
    // Z_raw_high = Wire.read();
    // Z_raw_low = Wire.read();

    AX_raw = (Wire.read() << 8 | Wire.read()); // X-axis value 16bit
    AY_raw = (Wire.read() << 8 | Wire.read()); // Y-axis value
    AZ_raw = (Wire.read() << 8 | Wire.read()); // Z-axis value

    // Serial1.print("X_high = ");
    // // Serial1.printf("byte %d = ", i);
    // Serial1.println(X_raw_high);
    // Serial1.print("X_low = ");
    // Serial1.println(X_raw_low, HEX);

    // Serial1.print("Y_high = ");
    // Serial1.println(Y_raw_high, HEX);
    // Serial1.print("Y_low = ");
    // Serial1.println(Y_raw_low, HEX);

    // Serial1.print("Z_high = ");
    // Serial1.println(Z_raw_high, HEX);
    // Serial1.print("Z_low = ");
    // Serial1.println(Z_raw_low, HEX);
    Serial1.println(static_cast<float>(AX_raw) / 16384);
    Serial1.println(static_cast<float>(AY_raw) / 16384);
    Serial1.println(static_cast<float>(AZ_raw) / 16384);

    i--;
    Serial1.println("-----------------------------------------");
  }
  delay(1000);
}