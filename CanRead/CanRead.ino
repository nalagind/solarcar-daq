/*
This is simple example to read all data from CAN bus and print it out to serial bus.
*/

#include "STM32_CAN.h"
//STM32_CAN Can( CAN1, DEF );  //Use PA11/12 pins for CAN1.
STM32_CAN Can( CAN1, ALT );  //Use PB8/9 pins for CAN1.
//STM32_CAN Can( CAN1, ALT_2 );  //Use PD0/1 pins for CAN1.
//STM32_CAN Can( CAN2, DEF );  //Use PB12/13 pins for CAN2.
//STM32_CAN Can( CAN2, ALT );  //Use PB5/6 pins for CAN2
//STM32_CAN Can( CAN3, DEF );  //Use PA8/15 pins for CAN3.
//STM32_CAN Can( CAN3, ALT );  //Use PB3/4 pins for CAN3

HardwareSerial Serial1(PC5, PB10);

static CAN_message_t CAN_RX_msg;

void setup() {
  Serial1.begin(115200);
  pinMode(PA9, OUTPUT);
  digitalWrite(PA9, HIGH);
  Can.begin();
  //Can.setBaudRate(250000);  //250KBPS
  Can.setBaudRate(500000);  //500KBPS
  //Can.setBaudRate(1000000);  //1000KBPS
  Serial1.println("ok");
}

void loop() {
  if (Can.read(CAN_RX_msg) ) {
    digitalWrite(PA9, LOW);

    Serial1.print("Channel:");
    Serial1.print(CAN_RX_msg.bus);
    if (CAN_RX_msg.flags.extended == false) {
      Serial1.print(" Standard ID:");
    }
    else {
      Serial1.print(" Extended ID:");
    }
    Serial1.print(CAN_RX_msg.id, HEX);

    Serial1.print(" DLC: ");
    Serial1.print(CAN_RX_msg.len);
    if (CAN_RX_msg.flags.remote == false) {
       Serial1.print(" buf: ");
      for(int i=0; i<CAN_RX_msg.len; i++) {
        Serial1.print("0x"); 
        Serial1.print(CAN_RX_msg.buf[i], HEX); 
        if (i != (CAN_RX_msg.len-1))  Serial1.print(" ");
      }
      Serial1.println();
    } else {
       Serial1.println(" Data: REMOTE REQUEST FRAME");
    }
    digitalWrite(PA9, HIGH);
  }
}