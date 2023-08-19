// CANCommands.cpp

#include "CANCommands.h"

void initCAN() {
	initializeMessageList();
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
    while (Can.available()) {
        CAN_message_t msg;
        Can.read(msg);
        processReceivedMessage(msg);
    }
}

string processReceivedMessage(const CAN_message_t& msg) {
  
    String output = "";
    DateTime now = rtc.now();

    // Prepare the timestamp
    output += "Timestamp: ";
    output += now.year();
    output += "/";
    output += now.month();
    output += "/";
    output += now.day();
    output += " ";
    output += now.hour();
    output += ":";
    output += now.minute();
    output += ":";
    output += now.second();
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


