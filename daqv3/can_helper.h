#include "pins_arduino_analog.h"
#include "itoa.h"
#include <cstdio>
#include <sys/_stdint.h>
#pragma once
#include "STM32_CAN.h"
#include <STM32RTC.h>

typedef void (*funcPointer)(const CAN_message_t& msg, char* interpretation);

extern int record_sn;
extern STM32RTC& rtc;

struct CAN_node {
  uint32_t id;
  const char* name;
  funcPointer data_interpreter;
  const char* info;
};

String processReceivedMessage(const CAN_message_t& msg, CAN_node node) {
  String output = "";
  
  // time
  char strbuf[42] = "";
  sprintf(strbuf, "%04d/%02d/%02d %02d:%02d:%02d", rtc.getYear() + 2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  output += strbuf;
  output += ",";
  strbuf[0] = 0;

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
  if (msg.flags.extended == false) {
      output += " Standard ID: ";
  } else {
      output += " Extended ID: ";
  }
  output += msg.id;
  output += " ";
  output += "DLC: ";
  output += msg.len;
  output += ",";

  // can data
  // if (msg.flags.remote == false) {
  //     output += "buf: ";
  //     for (int i = 0; i < msg.len; i++) {
  //         output += "0x";
  //         output += msg.buf[i];
  //         if (i != (msg.len - 1)) output += " ";
  //     }
  // } else {
  //     output += "Data: REMOTE REQUEST FRAME";
  // }
  node.data_interpreter(msg, strbuf);
  output += strbuf;
  output += ",";
  strbuf[0] = 0;

	// telemetry
  output += ",";

  // source info
  output += node.name;

  // sn
  output += record_sn;
  output += ",";
  record_sn++;

  // info
  output += node.info;
  output += "\n";

	return output;
}

void read_accel(const CAN_message_t& msg, char* interpretation) {
  int16_t accel[3];

  accel[0] = msg.buf[0] << 8 | msg.buf[1];
  accel[1] = msg.buf[2] << 8 | msg.buf[3];
  accel[2] = msg.buf[4] << 8 | msg.buf[5];

  sprintf(interpretation, "%d %d %d", accel[0], accel[1], accel[2]);
}

CAN_node daq_susp_accel {
  .id = 0x1A5,
  .name = "DAQ_susp_accel",
  .data_interpreter = read_accel,
  .info = "suspension DAQ accelerometer"
};

CAN_node CAN_node_lookup[] = {
  daq_susp_accel,
};

void read_generic(const CAN_message_t& msg, char* interpretation) {
  strcat(interpretation, "buf: ");
  if (msg.flags.remote == false) {
    for (int i = 0; i < msg.len; i++) {
        strcat(interpretation, "0x");
        sprintf(interpretation, "%x", msg.buf[i]);
        if (i != (msg.len - 1)) strcat(interpretation, " ");
    }
  } else {
      strcat(interpretation, "Data: REMOTE REQUEST FRAME");
  }
}

const uint16_t nodes_count = sizeof(CAN_node_lookup) / sizeof(CAN_node);

CAN_node identify_CAN_node(uint32_t id) {
  for (int i = 0; i < nodes_count; i++) {
    if (CAN_node_lookup[i].id == id) {
      return CAN_node_lookup[i];
    }
  }

  CAN_node unknown {
    .id = id,
    .name = "unknown node",
    .data_interpreter = read_generic,
    .info = "unknown node"
  };
  return unknown;
}
