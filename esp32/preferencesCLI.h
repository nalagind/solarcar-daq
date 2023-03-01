#pragma once

#include <SimpleCLI.h>
#include <Preferences.h>

#define ARG_SSID "wifi ssid,ssid"
#define ARG_PWD "wifi pswd,pwd"
#define ARG_INFLUXTOKEN "influx token,t"
#define ARG_INFLUXURL "influx URL,l"
#define ARG_LISTCONFIG "ls,list"
#define ARG_RESTART "restart,done"
#define ARG_CLEAR "clearall"

#define NOENTRY "noentry"

void configCmdCallback(cmd* c) {
  Command cmd(c);
  Preferences configStorage;
  bool notTrivial = false;

  int argCount = cmd.countArgs();

  if (cmd.getArg(ARG_RESTART).isSet()) {
    Serial.print("Load new configuration and restart in ");
    for (int i = 3; i > 0; i--) {
      Serial.printf("%d ", i);
      delay(1000);
    }
    esp_restart();
  }

  configStorage.begin("general");

  if (cmd.getArg(ARG_CLEAR).isSet()) {
    if (configStorage.clear()) {
      Serial.println("config cleared");
    } else {
      Serial.println("an error occured");
    }
    configStorage.end();
    return;
  }

  if (cmd.getArg(ARG_LISTCONFIG).isSet()) {
    Serial.println("\nDAQ CONFIG MENU\nitem\t\t\tset with\tcurrent value");
    Serial.println("-----------------------------------------------------------");

    for (int i = 0; i < argCount - 3; i++) {
      Argument arg = cmd.getArg(i);
      String argName = arg.getName();

      String s = "not set";
      String value = configStorage.getString(argName.c_str(), s);
      Serial.printf("%s\t\t-%s\t\t%s\n"
        , argName.substring(0, argName.indexOf(','))
        , argName.substring(argName.indexOf(',') + 1)
        , value);
    }
    Serial.print("\n\n");
    
    configStorage.end();
    return;
  }

  for (int i = 0; i < argCount - 3; i++) {
    Argument arg = cmd.getArg(i);
    String value = arg.getValue();
    if (value != NOENTRY) {
      notTrivial = true;
      String argName = arg.getName();

      if (configStorage.putString(argName.c_str(), value.c_str()) > 0) {
        Serial.printf("successfully saved %s: %s\n"
          , argName.substring(0, argName.indexOf(','))
          , value);
      } else {
        Serial.println("an error occurred saving configuration, please try again");
      }
    }
  }
  configStorage.end();
  
  if (!notTrivial) {
    Serial.println("DAQ CONFIG MENU");
    Serial.println("use \"config -ls\" to view all options\n");
  }
}

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

SimpleCLI setupCLI() {
  SimpleCLI cli;
  Command config;

  cli.setOnError(errorCallback);
	config = cli.addCommand("config", configCmdCallback);
    // config = cli.addCommand("config");
	config.addArg(ARG_SSID, NOENTRY);
	config.addArg(ARG_PWD, NOENTRY);
	config.addArg(ARG_INFLUXTOKEN, NOENTRY);
	config.addArg(ARG_INFLUXURL, NOENTRY);
	config.addFlagArg(ARG_LISTCONFIG);
	config.addFlagArg(ARG_RESTART);
	config.addFlagArg(ARG_CLEAR);

  return cli;
}