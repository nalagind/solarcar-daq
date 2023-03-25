#pragma once

#include <SimpleCLI.h>
#include <Preferences.h>

#define ARG_SSID "wifi ssid,w"
#define ARG_PWD "wifi pswd,p"
#define ARG_INFLUXTOKEN "influx token,t"
#define ARG_INFLUXURL "influx URL,l"
#define ARG_INFLUXBUCKET "influx buckt,b"
// #define ARG_INFLUXORG "influx org, o"
// #define ARG_TZINFO "tzinfo,tz"
// #define ARG_CANFREQ "can freq, fr"
#define ARG_SDFILENAME "sd filename,n"
// #define ARG_PROFILESAVE "save"
// #define ARG_PROFILELOAD "load"
// #define ARG_PROFILEDELETE "delete"
#define ARG_SDOVERWRITE "sd overwrite,o"
#define ARG_CANMODE "can loopback,m"

#define ARG_LISTCONFIG "ls,list"
#define ARG_RESTART "restart,done"
#define ARG_CLEAR "clearall"

// #define PROFILE_DEFAULT "general"
#define CANMODE_LOOPBACK "loopback"
#define NOENTRY "noentry"

#define FOLDERNAME "/Solar Car Trip Data"
#define FILENAME FOLDERNAME"/data.csv"

extern bool WIFI_SET;
extern bool INFLUX_SET;
extern bool CAN_SET;

void restart() {
  Serial.print("Load new configuration and restart in ");
  for (int i = 3; i > 0; i--) {
    Serial.printf("%d ", i);
    delay(1000);
  }
  esp_restart();
}

void configCmdCallback(cmd* c) {
  Command cmd(c);
  Preferences configStorage;
  bool notTrivial = false;

  int argCount = cmd.countArgs();

  if (cmd.getArg(ARG_RESTART).isSet()) restart();

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
    Serial.println("----------------------------------------------------------------");

    for (int i = 0; i < argCount - 3; i++) {
      Argument arg = cmd.getArg(i);
      String argName = arg.getName();

      String value = configStorage.getString(argName.c_str(), String("not set or using default"));
      Serial.printf("%s\t\t-%s\t\t"
        , argName.substring(0, argName.indexOf(','))
        , argName.substring(argName.indexOf(',') + 1));
      Serial.println(value);
    }
    Serial.print("\n\n");
    
    configStorage.end();
    return;
  }

  if (cmd.getArg(ARG_CANMODE).isSet()) {
    String argName = String(ARG_CANMODE);
    if (configStorage.getString(ARG_CANMODE) == CANMODE_LOOPBACK) {
      configStorage.remove(ARG_CANMODE);
      Serial.println("can is now in normal");
    } else {
      if (configStorage.putString(ARG_CANMODE, CANMODE_LOOPBACK) > 0) {
        Serial.println("can is now in loopback");
      } else {
        Serial.println("an error occurred saving configuration, please try again");
      }
    }
    
    CAN_SET = true;
    Serial.println("ok");
    configStorage.end();
    return;
  }

  for (int i = 0; i < argCount - 3; i++) {
    Argument arg = cmd.getArg(i);
    String value = arg.getValue();
    if (value != NOENTRY) {
      notTrivial = true;
      String argName = arg.getName();
      const char* argn = argName.c_str();

      if (configStorage.putString(argn, value.c_str()) > 0) {
        Serial.printf("successfully saved %s, ", argName.substring(0, argName.indexOf(',')));
        Serial.println(value);
        
        if (strstr(argn, "wifi") != NULL)
          WIFI_SET = true;
        else if (strstr(argn, "influx") != NULL)
          INFLUX_SET = true;
        else if (strstr(argn, "can") != NULL)
          CAN_SET = true;
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
	config.addArg(ARG_SSID, NOENTRY);
	config.addArg(ARG_PWD, NOENTRY);
	config.addArg(ARG_INFLUXTOKEN, NOENTRY);
	config.addArg(ARG_INFLUXURL, NOENTRY);
  config.addArg(ARG_INFLUXBUCKET, NOENTRY);
  config.addArg(ARG_SDFILENAME, NOENTRY);
  // config.addFlagArg(ARG_SDOVERWRITE, NOENTRY);
  config.addFlagArg(ARG_CANMODE, NOENTRY);
	config.addFlagArg(ARG_LISTCONFIG);
	config.addFlagArg(ARG_RESTART);
	config.addFlagArg(ARG_CLEAR);

  return cli;
}

String getConfigStorageString(const char* key) {
  Preferences p;
  p.begin("general");
  String v = p.getString(key);
  p.end();
  return v;
}

String wifi_SSID() {
  return getConfigStorageString(ARG_SSID);
}

String wifi_password() {
  return getConfigStorageString(ARG_PWD);
}

String influx_URL() {
  return getConfigStorageString(ARG_INFLUXURL);
}

String influx_token() {
  return getConfigStorageString(ARG_INFLUXTOKEN);
}

String influx_bucket() {
  return getConfigStorageString(ARG_INFLUXBUCKET);
}

String sd_filename() {
  String n = getConfigStorageString(ARG_SDFILENAME);
  if (n == "") return String(FILENAME);
  else return (String(FOLDERNAME) + "/" + n);
}

bool can_is_loopback() {
  if (getConfigStorageString(ARG_CANMODE) == CANMODE_LOOPBACK)
    return true;
  else return false;
}