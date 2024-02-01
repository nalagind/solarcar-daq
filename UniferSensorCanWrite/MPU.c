#include<Wire.h>
#include "STM32_CAN.h"
int UNIF_LED = PC7;

const int MPU_addr=0x68;
int16_t AX_raw,AY_raw,AZ_raw;
int16_t AX,AY,AZ;
static CAN_message_t CAN_TX_msg;
byte X_raw_high, X_raw_low, Y_raw_high, Y_raw_low, Z_raw_high, Z_raw_low;

STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.

//                      RX    TX
HardwareSerial Serial1(PA3, PA2);
void setup() {
  delay(1000);
  Serial1.begin(115200);
  pinMode(UNIF_LED, OUTPUT);

  Wire.setSDA(PC9);
  Wire.setSCL(PA8);
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B); // WHO_AM_I register address
  Wire.write(0x00);
  Wire.endTransmission();
  Can.begin();
  Can.setBaudRate(500000);
}

void loop() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B); //0x3B is the accel address
  Wire.endTransmission();
  Wire.requestFrom(MPU_addr,6);

  // AX_raw = (Wire.read() << 8 | Wire.read()); // X-axis value 16bit
  // AY_raw = (Wire.read() << 8 | Wire.read()); // Y-axis value
  // AZ_raw = (Wire.read() << 8 | Wire.read()); // Z-axis value

  X_raw_high = Wire.read();
  X_raw_low = Wire.read();
  Y_raw_high = Wire.read();
  Y_raw_low = Wire.read();
  Z_raw_high = Wire.read();
  Z_raw_low = Wire.read();

  CAN_TX_msg.id = (0x1A5);
  CAN_TX_msg.len = 6;
  CAN_TX_msg.buf[0] = X_raw_high;
  CAN_TX_msg.buf[1] = X_raw_low;
  CAN_TX_msg.buf[2] = Y_raw_high;
  CAN_TX_msg.buf[3] = Y_raw_low;
  CAN_TX_msg.buf[4] = Z_raw_high;
  CAN_TX_msg.buf[5] = Z_raw_low;
  Can.write(CAN_TX_msg);

  Serial1.print("X_high = ");
  Serial1.println(X_raw_high, HEX);
  Serial1.print("X_low = ");
  Serial1.println(X_raw_low, HEX);

  Serial1.print("Y_high = ");
  Serial1.println(Y_raw_high, HEX);
  Serial1.print("Y_low = ");
  Serial1.println(Y_raw_low, HEX);

  Serial1.print("Z_high = ");
  Serial1.println(Z_raw_high, HEX);
  Serial1.print("Z_low = ");
  Serial1.println(Z_raw_low, HEX);
 
  Serial1.println("-----------------------------------------");
  delay(5000);
}