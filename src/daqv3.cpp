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
int tx_state = RADIOLIB_ERR_NONE;

SdFs SD;

TinyGPSPlus gps;

SimpleCLI cli = setupCLI();
Preferences pref;

HardwareSerial SerialGPS(PA3, PA2);
BufferedPrintPlus<Print, 1> sbp(&Serial);
BufferedPrintPlus<Print, 1> sbp2(&Serial);
BufferedPrintPlus<FsFile, 255> sdbp;
BufferedPrintPlus<FsFile, 255> binbp;

uint32_t t = 0;
bool real_time = false;
volatile bool set_time = false;
uint8_t second;
OperatingMode op_mode = SOLAR_CAR;
uint32_t countdown;
String input;
bool sbp_e;
bool sbp2_e;

uint32_t gps_last;

Sys_Stat sys;

CSV_Header filter[] = {gps_spd_kmph, can_ID, sn, timestamp};

void pps_callback() {
  digitalToggle(PB6);
  if (set_time) {
    uint8_t s = (second + 1) % 60;
    if (s == 0) real_time = false;
    else rtc.setSeconds(s);
    set_time = false;
  }
}

void setup() {
  Serial.setRx(PC5);
  Serial.setTx(PB10);
  Serial.begin(115200);

  pinMode(PA4, INPUT);
  attachInterrupt(PA4, pps_callback, RISING);

  rtc.setClockSource(STM32RTC::HSE_CLOCK);
  rtc.begin(STM32RTC::HOUR_24);

  EEPROM.get(0, pref);
  
  Can.begin();
  Can.setBaudRate(pref.can_rate * 1000);

  sys.update(SD_Init, sd_begin(PC4, PA6, PA7, PA5));
  sys.update(SD_CSV, file_open(file, "daq.csv", FILE_OVERWRITE));
  sys.update(SD_BIN, file_open(file_bin, "daq.bin", FILE_OVERWRITE));

  sdbp.begin(&file);
  binbp.begin(&file_bin);
  file.sync();
  file_bin.sync();
  
  write_header(sdbp);

  SerialGPS.begin(9600);

  sys.update(SC_Radio, lora_init(pref.lora_frequency, pref.lora_bandwidth, pref.lora_spreading_factor, pref.lora_coding_rate, pref.lora_CRC));
  radio.setPacketSentAction(radio_txCpltCallback);
  
  Serial.println("started");

  sbp.enable_write(pref.sbp_enable == 1);
  sbp.println("ok");
  sbp2.enable_write(true);
  sdbp.config_sync(true, pref.sdbp_sync);
  binbp.config_sync(true, pref.sdbp_sync);
  CSV_Line::make_filter(filter);
}

void loop() {
  if (Serial.available() && op_mode == SOLAR_CAR) {
    sbp_e = sbp.enable_write(false); sbp2_e = sbp2.enable_write(false);
    input = "";
    countdown = millis();
    sys.to_cli();
    Serial.printf("DAQ is continuing in the background\n"
                  "use \"config -ls\" to see all options\n"
                  "Returning in %d seconds\n", pref.startup_delay); 
    Serial.print("% ");
    op_mode = CLI_NO_BLOCK;
  }
    
  if (op_mode == CLI_NO_BLOCK && (millis() - countdown) < (pref.startup_delay * 1000)) {
    feed_cli(input);  
  } else if (op_mode == CLI_NO_BLOCK) {
    sbp.enable_write(sbp_e); sbp2.enable_write(sbp2_e);
    op_mode = SOLAR_CAR;
  }

  if (Can.read(CAN_RX_msg)) {
    CSV_Line logger;
    LogBlob log(CAN);
    log.can_rx_msg = CAN_RX_msg;
    log.to_csv_line(logger);
    logger.write_row(sbp, filter, true, false);
    logger.write_row(sdbp);
    log.write_bin(binbp);
    sbp.println();

    if (transmit_done) {
      radio.finishTransmit();
      if (tx_state != RADIOLIB_ERR_NONE) LogBlob err_txend(Err);

      if (radio.startTransmit(reinterpret_cast<uint8_t*>(&log), sizeof(LogBlob)) != RADIOLIB_ERR_NONE) {
        LogBlob err_txstart(Err);
      }
      transmit_done = false;
    }
    
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
      if (gps.time.isValid() && !real_time) {
        rtc.setYear(gps.date.year() - 2000);
        rtc.setMonth(gps.date.month());
        rtc.setDay(gps.date.day());
        rtc.setHours((gps.time.hour() + pref.timezone_offset) % 24);
        rtc.setMinutes(gps.time.minute());
        second = gps.time.second();
        rtc.setSeconds(second);
        set_time = true;
        real_time = true;
        sys.update(WorldTime, true);
      }

      LogBlob gps_blob(LogType::GPS);
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
      gps_blob.write_bin(binbp);
      sbp.println();
      sys.update(SC_GPS, true);
    }
    
    t = millis();
  }

  if (millis() - gps_last > 5000 && gps.charsProcessed() < 10) {
    sys.update(SC_GPS, false);
    gps_last = millis();
  }
}
