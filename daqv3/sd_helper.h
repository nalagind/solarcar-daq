#pragma once
#include <SdFat.h>

#define SPI_CLOCK SD_SCK_MHZ(50)

extern SdFat SD;
File file;

bool appendFile(const char *path, const char *message) {
  file = SD.open(path, FILE_WRITE); //FILEWRITE includes flags for creating file, appending, and R&W
  if (!file) {
    Serial.println("Failed to open file for appending");
    return false;
  }

  if (!file.println(message)) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool writeFile(const char *path, const char *message) {
  file = SD.open(path, O_RDWR | O_CREAT);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }

  if (!file.print(message)) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool sd_init(uint32_t chipSelectPin = PC4, uint32_t miso = PA6, uint32_t mosi = PA7, uint32_t sclk = PA5, const char *path = "daq.csv") {
  //need to change SPI pins for timeconsumer board
  SPI.setMISO(miso); //PA6
  SPI.setMOSI(mosi); //PA7
  SPI.setSCLK(sclk); //PA5

  if (!SD.begin(SdSpiConfig(chipSelectPin, DEDICATED_SPI, SPI_CLOCK))) { //PC4
    Serial.println("SD initialization failed!");
    return false;
  }

  if (!writeFile(path, "time,registrar,CAN id,CAN data,telemetry,source,sn,info\n")) {
    Serial.println("Writing to file failed!");
    return false;
  }
  Serial.println("...and mounted");
  return true;
}