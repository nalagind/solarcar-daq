#include "pins_arduino_analog.h"
#include "itoa.h"
#include <cstdio>
#include <sys/_stdint.h>
#pragma once
#include "STM32_CAN.h"
#include <STM32RTC.h>
#include "csv_logger.h"
#include "can_nodes.h"

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