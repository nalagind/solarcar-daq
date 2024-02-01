#include "can_helper.h"
#include "lora_helper.h"
#include "sd_helper.h"
#include "CLICommands.h"
#include "RTC_helper.h"
#include "FSK_helper.h"
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
}

void loop() {
  if (Can.read(CAN_RX_msg)) {
    Serial.println("received");
    CAN_node node = identify_CAN_node(CAN_RX_msg.id);
    can_record = processReceivedMessage(CAN_RX_msg, node);
    Serial.println(can_record);
    
    if (pref.file_overwrite) {
      if (!writeFile(pref.filename, can_record.c_str())) {
        Serial.println("Writing to file failed");
      }
      Serial.println("record line written");
    } else {
      if (!appendFile(pref.filename, can_record.c_str())) {
        Serial.println("Writing to file failed");
      }
      Serial.println("record line written");
    }

    LoRaTransmit(can_record);
    // FSK_Transmit(can_record);
  }
}
