// CANCommands.cpp

#include "CANCommands.h"

/* Get the rtc object */
STM32RTC& rtc = STM32RTC::getInstance();

void initCAN() {
	  initializeMessageNameMap();
    Can.begin();
    Can.setBaudRate(250000);  // Set the desired baud rate
    //Can.setBaudRate(500000);  //500KBPS
    //Can.setBaudRate(1000000);  //1000KBPS
}

void CAN_SendMessage(uint32_t id, uint8_t len, uint8_t* data) {
    CAN_TX_msg.id = id;
    CAN_TX_msg.len = len;
    for (uint8_t i = 0; i < len; i++) {
        CAN_TX_msg.buf[i] = data[i];
    }
    Can.write(CAN_TX_msg);
}

void CAN_Read() {
    
}
String processReceivedMessage(const CAN_message_t& msg) {
  
    String output = "";

    // Prepare the timestamp
    output += "Timestamp: ";
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
    output += msg.bus;
    
    // Print the received message's name if it's in the map
    if (messageNameMap.count(msg.id)) {
        output += "\nMessage Name: ";
        output += messageNameMap[msg.id];
    }
    if (msg.flags.extended == false) {
        output += " \nStandard ID: ";
    } else {
        output += " \nExtended ID: ";
    }
    output += msg.id;
    output += "\nDLC: ";
    output += msg.len;

    if (msg.flags.remote == false) {
        output += "\nbuf: ";
        for (int i = 0; i < msg.len; i++) {
            output += "0x";
            output += msg.buf[i];
            if (i != (msg.len - 1)) output += " ";
        }
    } else {
        output += "\nData: REMOTE REQUEST FRAME";
    }
	
	return output;
}


