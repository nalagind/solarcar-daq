#include "can_helper.h"
#include "lora_helper.h"
#include "sd_helper.h"
#include "CLICommands.h"
#include "RTC_helper.h"
STM32_CAN Can(CAN1, ALT);
static CAN_message_t CAN_RX_msg;

SPIClass SPI_3(PC12, PC11, PC10);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);

SdFat SD;

String can_record;
int record_sn = 1;

SimpleCLI cli = setupCLI();

void setup() {
  Serial.setRx(PC5);
  Serial.setTx(PB10);
  Serial.begin(115200);

  Can.begin();
  Can.setBaudRate(500000);

  lora_init();

  sd_init(PC4, PA6, PA7, PA5, "data.txt");
  Serial.println("ok");
}

void loop() {
  if (Can.read(CAN_RX_msg) ) {
    can_record = processReceivedMessage(CAN_RX_msg);
    Serial.println(can_record);
    
    if (!writeFile("data.txt", can_record.c_str())) {
      Serial.println("Writing to file failed");
    }
    Serial.println("record line written");

    LoRaTransmit(can_record);
  }
}
