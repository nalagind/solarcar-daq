#include "can_helper.h"
#include "lora_helper.h"
// #include "sd_helper.h"
#include "CLICommands.h"
#include "RTC_helper.h"
#include "FSK_helper.h"
#include "logger.h"
#include "sd_helper.h"
STM32RTC& rtc = STM32RTC::getInstance();

STM32_CAN Can(CAN1, ALT);
static CAN_message_t CAN_RX_msg;

SPIClass SPI_3(PC12, PC11, PC10);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);

SdFat SD;

String can_record;
int record_sn = 1;

SimpleCLI cli = setupCLI();
Preferences pref;

void setup() {
  Serial.setRx(PC5);
  Serial.setTx(PB10);
  Serial.begin(115200);

  pinMode(PB6, OUTPUT);
  digitalWrite(PB6, LOW);

  rtc.begin();

  cli.parse("config -ls");

  uint16_t countdown = millis();
  Serial.println("starting in");
  while (millis() - countdown < pref.startup_delay * 1000) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      Serial.print("% ");
      input.trim();
      Serial.println(input);
      cli.parse(input);
      countdown = millis();
    }

    if ((millis() - countdown) % 1000 <= 3) {
      Serial.print(pref.startup_delay - (millis() - countdown) / 1000);
      Serial.print(" ");
      delay(5);
    }
  }

  Can.begin();
  Can.setBaudRate(pref.can_rate * 1000);

  sd_init(PC4, PA6, PA7, PA5, pref.filename);

  lora_init(pref.lora_frequency, pref.lora_bandwidth, pref.lora_spreading_factor, pref.lora_coding_rate, pref.lora_CRC);
  
  Serial.println("started");

  DAQBufferedPrint<Print, 32> sbp(&Serial);
}

void loop() {
  if (Can.read(CAN_RX_msg)) {
    Serial.println("received");
    CSV_Row logger(record_sn, LogType::CAN);
    process_CAN_msg(CAN_RX_msg, logger);
    // char buf[100];
    // CSV_Header descp_req[] = {can_ID, daq_susp_FL_acc_x, daq_susp_FL_acc_y, daq_susp_FL_acc_z};
    // logger.describe(buf, descp_req, 4);
    // Serial.println(buf);
    
    // if (pref.file_overwrite) {
    //   if (!writeFile(pref.filename, can_record.c_str())) {
    //     Serial.println("Writing to file failed");
    //   }
    //   Serial.println("record line written");
    // } else {
    //   if (!appendFile(pref.filename, can_record.c_str())) {
    //     Serial.println("Writing to file failed");
    //   }
    //   Serial.println("record line written");
    // }

    // LoRaTransmit(can_record);
    // FSK_Transmit(can_record);
    record_sn++;
  }
}
