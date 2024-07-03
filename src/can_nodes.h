#pragma once

#include "logger.h"

typedef void (*funcPointer)(const CAN_message_t& msg, CSV_Row& logger);

struct CAN_node {
  uint32_t id;
  const char* name;
  funcPointer data_interpreter;
  const char* info;
};

// void read_accel(const CAN_message_t& msg, CSV_Row& logger) {
//   int16_t accel_raw[3];
//   float accel[3];

//   accel_raw[0] = msg.buf[0] << 8 | msg.buf[1];
//   accel_raw[1] = msg.buf[2] << 8 | msg.buf[3];
//   accel_raw[2] = msg.buf[4] << 8 | msg.buf[5];

//   accel[0] = (float)accel_raw[0] / 16384;
//   accel[1] = (float)accel_raw[1] / 16384;
//   accel[2] = (float)accel_raw[2] / 16384;
  
//   logger.append(can_node_name, "daq_susp_FL");
//   logger.append(daq_susp_FL_acc_x, accel[0]);
//   logger.append(daq_susp_FL_acc_y, accel[1]);
//   logger.append(daq_susp_FL_acc_z, accel[2]);
// }

// CAN_node daq_susp_accel {
//   .id = 0x1A5,
//   .name = "susp_acc",
//   .data_interpreter = read_accel,
//   .info = "suspension DAQ accel"
// };

CAN_node CAN_nodes[] = {
  // daq_susp_accel,
};

const uint16_t nodes_count = sizeof(CAN_nodes) / sizeof(CAN_node);

CAN_node identify_CAN_node(uint32_t id) {
  for (int i = 0; i < nodes_count; i++) {
    if (CAN_nodes[i].id == id) {
      return CAN_nodes[i];
    }
  }

  CAN_node unknown {
    .id = id,
    .name = nullptr,
    .data_interpreter = nullptr,
    .info = nullptr
  };
  return unknown;
}
