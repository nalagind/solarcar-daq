struct CAN_node {
  uint32_t id;
  const char* name;
  funcPointer data_interpreter;
  const char* info;
};

CAN_node CAN_node_lookup[] = {
  daq_susp_accel,
};

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