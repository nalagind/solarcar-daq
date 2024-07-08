#include <Arduino.h>

#include "can_helper.h"
#include "lora_helper.h"
// #include "sd_helper.h"
#include "CLICommands.h"
#include "RTC_helper.h"
#include "FSK_helper.h"
#include "csv_logger.h"
#include "blob.h"
#include "sd_helper.h"
#include "TinyGPSPlus.h"

STM32RTC& rtc = STM32RTC::getInstance();

STM32_CAN Can(CAN1, ALT);
static CAN_message_t CAN_RX_msg;

SPIClass SPI_3(PC12, PC11, PC10);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);

SdFs SD;

TinyGPSPlus gps;

SimpleCLI cli = setupCLI();
Preferences pref;

HardwareSerial SerialGPS(PA3, PA2);
BufferedPrintPlus<Print, 1> sbp(&Serial);
BufferedPrintPlus<FsFile, 255, true, 256> sdbp;

uint32_t count = 0;
uint32_t t = 0;
volatile bool set_time = false;
bool real_time = false;
uint8_t second;

void pps_callback() {
  digitalToggle(PB6);
  if (set_time == true) {
    rtc.setSeconds((second + 1) % 60);
    set_time = false;
  }
}

void setup() {
  Serial.setRx(PC5);
  Serial.setTx(PB10);
  Serial.begin(115200);

  pinMode(PB6, OUTPUT);
  digitalWrite(PB6, HIGH);

  pinMode(PA4, INPUT);
  attachInterrupt(PA4, pps_callback, RISING);

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

  sd_init(PC4, PA6, PA7, PA5);
  write_header(sdbp);

  SerialGPS.begin(9600);

  lora_init(pref.lora_frequency, pref.lora_bandwidth, pref.lora_spreading_factor, pref.lora_coding_rate, pref.lora_CRC);
  
  Serial.println("started");

  sbp.enable_write(pref.sbp_enable == 1);
  sbp.println("ok");
}

void loop() {
  if (Can.read(CAN_RX_msg)) {
    sbp.println("can msg");
    CSV_Line logger;
    LogBlob log(CAN);
    log.can_rx_msg = CAN_RX_msg;
    // process_CAN_msg(CAN_RX_msg, logger);
    // CSV_Header descp_req[] = {can_ID, daq_susp_FL_acc_x, daq_susp_FL_acc_y, daq_susp_FL_acc_z};
    // logger.append(CSV_Header::can_ID, CAN_RX_msg.id);
    log.to_csv_line(logger);
    logger.write_row(sbp, true, false);
    logger.write_row(sdbp);
    sbp.println();
    
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
  }

  while (SerialGPS.available()) {
    gps.encode(SerialGPS.read());
  }
  if (millis() - t > 1000) {
    if (gps.satellites.value() > 1 && gps.location.isValid()) {
      if (real_time == false && gps.time.isValid()) {
        set_time = true;
        rtc.setYear(gps.date.year() - 2000);
        rtc.setMonth(gps.date.month());
        rtc.setDay(gps.date.day());
        rtc.setHours((gps.time.hour() + pref.timezone_offset) % 24);
        rtc.setMinutes(gps.time.minute());
        second = gps.time.second();
        real_time = true;
      }

      LogBlob gps_blob(GPS);
      gps_blob.gps_log = {
        static_cast<int>(gps.satellites.isValid() ? gps.satellites.value() : -1),
        static_cast<float>(gps.hdop.isValid() ? gps.hdop.hdop() : -1),
        static_cast<float>(gps.location.isValid() ? gps.location.lat() : -1),
        static_cast<float>(gps.location.isValid() ? gps.location.lng() : -1),
        static_cast<int>(gps.location.isValid() ? gps.location.age() : -1),
        static_cast<int>(gps.altitude.isValid() ? gps.altitude.meters() : -1),
        static_cast<float>(gps.speed.isValid() ? gps.speed.kmph() : -1),
      };
      CSV_Line l;
      gps_blob.to_csv_line(l);
      l.write_row(sbp, true, false);
      l.write_row(sdbp);
      sbp.println();
    }
    
    t = millis();
  }

  // if (count % 1000 == 0) sbp.println(count);
  // sdbp.printField(count++, '\n');
  // delay(5);
}
