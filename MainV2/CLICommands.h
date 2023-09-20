
#include <SimpleCLI.h>
#include "CANCommands.h"
#include "SDHelper.h" // Include the SDHelper header

extern SimpleCLI cli;
extern Command sendRequestCommand;
OperatingMode currentMode = SOLAR_CAR_MODE; // Default to solar car mode

enum OperatingMode {
    SOLAR_CAR_MODE,
    TRACE_CAR_MODE
};

void errorCallback(cmd_error* e) {
    CommandError cmdError(e);

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand()) {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}
void testCmdCallback(cmd* c) {
    Command cmd(c);
    if (currentMode == SOLAR_CAR_MODE) {

    if (cmd.getArg("sd_write").isSet()) {
      writeFile("/test.txt", "Testing SD write functionality.\n");
      Serial.println("Test message written to SD card.");
    }
    if (cmd.getArg("sd_read").isSet()) {
      Serial.println("Reading content from test.txt on SD card:");
      readFile("/test.txt");
    }
    if (cmd.getArg("CAN_write").isSet()) {
      CAN_TX_msg.id = (0x1A5);
      CAN_TX_msg.len = 8;
      CAN_TX_msg.buf[0] =  0x03;
      CAN_TX_msg.buf[1] =  0x41;
      CAN_TX_msg.buf[2] =  0x11;
      CAN_TX_msg.buf[3] =  0x21;
      CAN_TX_msg.buf[4] =  0x00;
      CAN_TX_msg.buf[5] =  0x00;
      CAN_TX_msg.buf[6] =  0x00;
      CAN_TX_msg.buf[7] =  0x00;
    
      Can.write(CAN_TX_msg);
    }
    } else if (currentMode == TRACE_CAR_MODE) {
        // Handle trace car mode commands if needed
    }


    }

}

void configCmdCallback(cmd* c) {
  Command cmd(c);

    if (cmd.getArg("restart").isSet()) {
    Serial.print("Load new configuration and restart in ");
    for (int i = 3; i > 0; i--) {
      Serial.printf("%d ", i);
      delay(1000);
    }
    //NVIC_SystemReset();
    HAL_NVIC_SystemReset();
  }

}
void send_request_CmdCallback(cmd* c) {
  Command cmd(c);
    if (cmd.getArg() > 0) {
        int wheel = int (cmd.getArg()); // The first argument specifies the wheel (0 to 3)
        // Convert the wheel number to the appropriate Request Command ID
        int requestCommandID = 0x08F89540 | (wheel << 4); // Assuming the IDs follow a pattern
        
        // Create and send the Request Command message
        uint8_t requestData[] = { 0x05 }; // Data to send
        CAN_SendMessage(requestCommandID, sizeof(requestData), requestData);
    } else {
        Serial.println("Missing argument. Usage: send-request <wheel_number>");
  }
}

void setModeCmdCallback(cmd* c) {
    Command cmd(c);
    if (cmd.getArg("solar_car").isSet()) {
        currentMode = SOLAR_CAR_MODE;
        
    } else if (cmd.getArg("trace_car").isSet()) {
        currentMode = TRACE_CAR_MODE;
        
    }
    Serial.print("Switched to ");
    Serial.println(currentMode == SOLAR_CAR_MODE ? "solar car mode" : "trace car mode");
}


SimpleCLI setupCLI() {

    SimpleCLI cli;
    cli.setOnError(errorCallback);

    Command config;
    config = cli.addCommand("config", configCmdCallback);
    config.addFlagArg("restart");
    
    Command test;
    test = cli.addCommand("test", testCmdCallback);
    test.addArg("sd_write","noentry");
    test.addArg("sd_read","noentry");
    test.addArg("CAN_write","noentry");

    Command send_request;
    send_request = cli.addCmd("send_request", send_request_CmdCallback);

    // Add mode command to switch between modes
    Command mode = cli.addCommand("mode", setModeCmdCallback);
    mode.addArg("solar_car", "noentry");
    mode.addArg("trace_car", "noentry");
    

  return cli;
}


