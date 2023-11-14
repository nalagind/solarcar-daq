#pragma once

#include <SimpleCLI.h>
#include "RTC_helper.h"
#include <EEPROM.h>

#define ARG_DATETIME "date time"
#define ARG_CAN_RATE "can rate(k),cr"
#define ARG_LORA_FREQ "lora freq(M),lf"
#define ARG_LORA_BANDWIDTH "lora bdwdth,bw"
#define ARG_LORA_SF "lora SF,sf"
#define ARG_LORA_CR "lora CR,cr"
#define ARG_LORA_CRC "lora CRC,crc"
#define ARG_FILE_OVERWRITE "overwrite,ow"
#define ARG_FILENAME "filename,fn"
#define ARG_YEAR "year,yr"
#define ARG_MONTH "month,mo"
#define ARG_DAY "day,day"
#define ARG_HOUR "hour,hr"
#define ARG_MIN "minute,min"
#define ARG_SEC "second,sec"

#define ARG_RESTART "restart"
#define ARG_LISTCONFIG "ls,list"

#define NOENTRY "noentry"

enum OperatingMode {
    SOLAR_CAR_MODE,
    TRACE_CAR_MODE
};

struct Preferences {
  uint16_t can_rate;
  float_t lora_frequency;
  uint16_t lora_bandwidth;
  uint8_t lora_spreading_factor;
  uint8_t lora_coding_rate;
  uint8_t lora_CRC;
  bool file_overwrite;
  char filename[32];
};

extern OperatingMode currentMode;
extern Preferences pref;

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

void configCmdCallback(cmd* c) {
  Command cmd(c);
  bool nonTrivial = false;

  if (cmd.getArg("restart").isSet()) {
    Serial.print("Load new configuration and restart in ");
    for (int i = 3; i > 0; i--) {
      Serial.printf("%d ", i);
      delay(1000);
    }
    //NVIC_SystemReset();
    HAL_NVIC_SystemReset();
  }
  
  EEPROM.get(0, pref);

  uint16_t can_rate = pref.can_rate;
  float lora_frequency = pref.lora_frequency;
  uint16_t lora_bandwidth = pref.lora_bandwidth;
  uint8_t lora_spreading_factor = pref.lora_spreading_factor;
  uint8_t lora_coding_rate = pref.lora_coding_rate;
  uint8_t lora_CRC = pref.lora_CRC;
  bool file_overwrite = pref.file_overwrite;
  char filename[32];
  strcpy(filename, pref.filename);

  if (cmd.getArg(ARG_LISTCONFIG).isSet()) {
    Serial.println("\nDAQ CONFIG MENU\nitem   set with   current value");
    Serial.println("-----------------------------------------------------------");

    for (int i = 0; i < cmd.countArgs() - 2; i++) {
      Argument arg = cmd.getArg(i);
      String argName = arg.getName();
      const char* argn = argName.c_str();

      // String s = "not set";
      char buf[64];
      sprintf(buf, "%s   -%s   "
        , argName.substring(0, argName.indexOf(',')).c_str()
        , argName.substring(argName.indexOf(',') + 1).c_str());
      Serial.print(buf);
      buf[0] = 0;

      if (strstr(argn, "date") != NULL) {
        sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", rtc.getYear() + 2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
        Serial.println(buf);
        buf[0] = 0;
      }

      if (strstr(argn, "can rate") != NULL) {
        Serial.println(can_rate);
      }

      if (strstr(argn, "lora freq") != NULL) {
        Serial.println(lora_frequency);
      }

      if (strstr(argn, "bdwdth") != NULL) {
        Serial.println(lora_bandwidth);
      }

      if (strstr(argn, "SF") != NULL) {
        Serial.println(lora_spreading_factor);
      }

      if (strstr(argn, "CR") != NULL) {
        Serial.println(lora_coding_rate);
      }

      if (strstr(argn, "CRC") != NULL) {
        Serial.println(lora_CRC);
      }

      if (strstr(argn, "overwrite") != NULL) {
        Serial.println(file_overwrite);
      }
      
      if (strstr(argn, "filename") != NULL) {
        Serial.println(filename);
      }
      
      if (strstr(argn, "year") || strstr(argn, "month") || strstr(argn, "day") || strstr(argn, "hour") || strstr(argn, "minute") || strstr(argn, "second") != NULL) {
        Serial.println();
      }
    }
    Serial.print("\n\n");
    
    return;
  }
 
  for (int i = 0; i < cmd.countArgs() - 2; i++) {
    Argument arg = cmd.getArg(i);
    String value = arg.getValue();
    
    if (value != NOENTRY) {
      nonTrivial = true;
      String argName = arg.getName();
      const char* argn = argName.c_str();

      if (strstr(argn, "can rate") != NULL) {
        pref.can_rate = value.toInt();
      }

      if (strstr(argn, "lora freq") != NULL) {
        pref.lora_frequency = value.toFloat();
      }

      if (strstr(argn, "bdwdth") != NULL) {
        pref.lora_bandwidth = value.toInt();
      }

      if (strstr(argn, "SF") != NULL) {
        pref.lora_spreading_factor = value.toInt();
      }

      if (strstr(argn, "CR") != NULL) {
        pref.lora_coding_rate = value.toInt();
      }

      if (strstr(argn, "CRC") != NULL) {
        pref.lora_CRC = value.toInt();
      }

      if (strstr(argn, "overwrite") != NULL) {
        pref.file_overwrite = arg.isSet();
      }
      
      if (strstr(argn, "filename") != NULL) {
        arg.getValue().toCharArray(pref.filename, 32);
      }

      if (strstr(argn, "year") != NULL) {
        rtc.setYear(value.toInt() - 2000);
      }

      if (strstr(argn, "month") != NULL) {
        rtc.setMonth(value.toInt());
      }

      if (strstr(argn, "day") != NULL) {
        rtc.setDay(value.toInt());
      }

      if (strstr(argn, "hour") != NULL) {
        rtc.setHours(value.toInt());
      }

      if (strstr(argn, "minute") != NULL) {
        rtc.setMinutes(value.toInt());
      }

      if (strstr(argn, "second") != NULL) {
        rtc.setSeconds(value.toInt());
      }
    }
  }

  Serial.print("Writing config");
  EEPROM.put(0, pref);
  Serial.println("...done");

  if (!nonTrivial) {
    Serial.println("DAQ CONFIG MENU");
    Serial.println("use \"config -ls\" to view all options\n");
  }
}
// void send_request_CmdCallback(cmd* c) {
//   Command cmd(c);
//     if (cmd.getArg() > 0) {
//         int wheel = int (cmd.getArg()); // The first argument specifies the wheel (0 to 3)
//         // Convert the wheel number to the appropriate Request Command ID
//         int requestCommandID = 0x08F89540 | (wheel << 4); // Assuming the IDs follow a pattern
        
//         // Create and send the Request Command message
//         uint8_t requestData[] = { 0x05 }; // Data to send
//         CAN_SendMessage(requestCommandID, sizeof(requestData), requestData);
//     } else {
//         Serial.println("Missing argument. Usage: send-request <wheel_number>");
//   }
// }


SimpleCLI setupCLI() {
  SimpleCLI cli;
  cli.setOnError(errorCallback);

  Command config;
  config = cli.addCommand("config", configCmdCallback);
  config.addArg(ARG_DATETIME, NOENTRY);
  config.addArg(ARG_CAN_RATE, NOENTRY);
  config.addArg(ARG_LORA_FREQ, NOENTRY);
  config.addArg(ARG_LORA_BANDWIDTH, NOENTRY);
  config.addArg(ARG_LORA_SF, NOENTRY);
  config.addArg(ARG_LORA_CR, NOENTRY);
  config.addArg(ARG_LORA_CRC, NOENTRY);
  config.addFlagArg(ARG_FILE_OVERWRITE);
  config.addArg(ARG_FILENAME, NOENTRY);
  config.addArg(ARG_YEAR, NOENTRY);
  config.addArg(ARG_MONTH, NOENTRY);
  config.addArg(ARG_DAY, NOENTRY);
  config.addArg(ARG_HOUR, NOENTRY);
  config.addArg(ARG_MIN, NOENTRY);
  config.addArg(ARG_SEC, NOENTRY);

  config.addFlagArg(ARG_LISTCONFIG);
  config.addFlagArg("restart");
    
  return cli;
}
