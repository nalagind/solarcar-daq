#include "STM32_CAN.h"
#include <STM32RTC.h>

STM32RTC& rtc = STM32RTC::getInstance();

String processReceivedMessage(const CAN_message_t& msg) {
  String output = "Timestamp: ";

  // timestamp
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
  output += ",";

  // source task name
  output += "can rx,";

  // Prepare message details
  // output += "\nChannel:";
  // output += msg.bus;
  
  // Print the received message's name if it's in the map
  // if (messageNameMap.count(msg.id)) {
  //     output += "\nMessage Name: ";
  //     output += messageNameMap[msg.id];
  // }

  // can id
  // if (msg.flags.extended == false) {
  //     output += " \nStandard ID: ";
  // } else {
  //     output += " \nExtended ID: ";
  // }
  output += msg.id;
  output += ",";
  // output += "\nDLC: ";
  // output += msg.len;

  // can data
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
  output += ",";

	// can telemetry
  output += ",";

  // source info
  output += ",";

  // sn
  output += record_sn;
  record += ",";
  record_sn++;

  // info
  record += ",";

	return output;
}

// std::map<uint32_t, String> messageNameMap;

// void initializeMessageNameMap() {
//   messageNameMap[0x10] = "HazardSignal";
//   messageNameMap[0x11] = "RightSignal";
//   messageNameMap[0x12] = "LeftSignal";
//   messageNameMap[0x13] = "FrontSignal";
//   messageNameMap[0x14] = "BackSignal";
//   messageNameMap[0x15] = "horn";
//   messageNameMap[0x02] = "cruizeCtrlSpeed";
//   messageNameMap[0x09] = "solarCellOutputEffPercent";
//   messageNameMap[0x02F4] = "Battery Status (BATT_ST)";
//   messageNameMap[0x04F4] = "Cell Voltage (CELL_VOLT)";
//   messageNameMap[0x05F4] = "Cell Temperature (CELL_TEMP)";
//   messageNameMap[0x07F4] = "Fault Information (ALM_INFO)";
//   messageNameMap[0x08850225] = "Frame 0 Rear Left Wheel";
//   messageNameMap[0x08850245] = "Frame 0 Rear Right Wheel";
//   messageNameMap[0x08850265] = "Frame 0 Front Left Wheel";
//   messageNameMap[0x08850285] = "Frame 0 Front Right Wheel";
//   messageNameMap[0x08950225] = "Frame 1 Rear Left Wheel";
//   messageNameMap[0x08950245]= "Frame 1 Rear Right Wheel";
//   messageNameMap[0x08950265]= "Frame 1 Front Left Wheel";
//   messageNameMap[0x08950285]= "Frame 1 Front Right Wheel";
//   messageNameMap[0x08A50225]="Frame 2 Rear Left Wheel";
//   messageNameMap[0x08A50245]= "Frame 2 Rear Right Wheel";
//   messageNameMap[0x08A50265]= "Frame 2 Front Left Wheel";
//   messageNameMap[0x08A50285]= "Frame 2 Front Right Wheel";
//   messageNameMap[0x08F89540] = "Request Command Rear Left Wheel";
//   messageNameMap[0x08F91540] = "Request Command Rear Right Wheel";
//   messageNameMap[0x08F99540] = "Request Command Front Left Wheel";
//   messageNameMap[0x08FA9540] = "Request Command Front Right Wheel";
// }