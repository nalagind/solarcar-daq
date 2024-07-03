#include "pins_arduino_analog.h"
#include "itoa.h"
#include <cstdio>
#include <sys/_stdint.h>
#pragma once
#include "STM32_CAN.h"
#include <STM32RTC.h>
#include "logger.h"
#include "can_nodes.h"

// String processReceivedMessage(const CAN_message_t& msg, CAN_node node) {
//   String output = "";
  
//   // time
//   char strbuf[42] = "";
//   sprintf(strbuf, "%04d/%02d/%02d %02d:%02d:%02d", rtc.getYear() + 2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
//   output += strbuf;
//   output += ",";
//   strbuf[0] = 0;

//   // source task name
//   output += "can rx,";

//   // Prepare message details
//   // output += "\nChannel:";
//   // output += msg.bus;
  
//   // Print the received message's name if it's in the map
//   // if (messageNameMap.count(msg.id)) {
//   //     output += "\nMessage Name: ";
//   //     output += messageNameMap[msg.id];
//   // }

//   // can id
//   if (msg.flags.extended == false) {
//       output += " Standard ID: ";
//   } else {
//       output += " Extended ID: ";
//   }
//   output += msg.id;
//   output += " ";
//   output += "DLC: ";
//   output += msg.len;
//   output += ",";

//   // can data
//   // if (msg.flags.remote == false) {
//   //     output += "buf: ";
//   //     for (int i = 0; i < msg.len; i++) {
//   //         output += "0x";
//   //         output += msg.buf[i];
//   //         if (i != (msg.len - 1)) output += " ";
//   //     }
//   // } else {
//   //     output += "Data: REMOTE REQUEST FRAME";
//   // }
//   node.data_interpreter(msg, strbuf);
//   output += strbuf;
//   output += ",";
//   strbuf[0] = 0;

// 	// telemetry
//   output += ",";

//   // source info
//   output += node.name;

//   // sn
//   output += record_sn;
//   output += ",";
//   record_sn++;

//   // info
//   output += node.info;
//   output += "\n";

// 	return output;
// }

void read_generic(const CAN_message_t& msg, CSV_Line& logger) {
  // if (msg.flags.remote == false) {
  //   for (int i = 0; i < msg.len; i++) {
  //       strcat(interpretation, "0x");
  //       sprintf(interpretation, "%x", msg.buf[i]);
  //       if (i != (msg.len - 1)) strcat(interpretation, " ");
  //   }
  // } else {
  //     strcat(interpretation, "Data: REMOTE REQUEST FRAME");
  // }

  logger.append(can_ID, msg.id, B_HEX);
  logger.append(can_DLC, msg.len);
  if (msg.flags.remote == true) logger.append(can_remote_request, 1);
  for (int i = 0; i < msg.len; i++) {
    if (msg.len > 8) break;
    logger.append(static_cast<CSV_Header>(static_cast<int>(CSV_Header::can_raw_D0) + i), msg.buf[i], B_HEX);
  }
}

void process_CAN_msg(const CAN_message_t& msg, CSV_Line& logger) {
    read_generic(msg, logger);
    CAN_node node = identify_CAN_node(msg.id);
    if (node.data_interpreter != nullptr) node.data_interpreter(msg, logger);
}