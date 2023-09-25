// CANCommands.h

#ifndef CAN_COMMANDS_H
#define CAN_COMMANDS_H

#include <Arduino.h>
#include <STM32_CAN.h>
#include "MessageList.h"
//#include <STM32RTC.h>

extern STM32_CAN Can;
extern CAN_message_t CAN_TX_msg;

void initCAN();
void CAN_SendMessage(uint32_t id, uint8_t len, uint8_t* data);
void CAN_Read();
String processReceivedMessage(const CAN_message_t& msg);

#endif
