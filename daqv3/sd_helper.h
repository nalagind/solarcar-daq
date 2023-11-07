#include <SdFat.h>

extern SdFat SD;
File file;

bool writeFile(const char *path, const char *message) {
  file = SD.open(path, FILE_WRITE); //FILEWRITE includes flags for creating file, appending, and R&W
  if (!file) {
    Serial.println("Failed to open file for writing, may not exist yet");
    return false;
  }

  if (!file.println(message)) {
    Serial.println("Write failed");
    file.close();
    return false;
  }

  file.close();
  Serial.println("File written");
  return true;
}

bool sd_init(uint32_t chipSelectPin, uint32_t miso, uint32_t mosi, uint32_t sclk, const char *path) {
  //need to change SPI pins for timeconsumer board
  SPI.setMISO(miso); //PA6
  SPI.setMOSI(mosi); //PA7
  SPI.setSCLK(sclk); //PA5

  if (!SD.begin(chipSelectPin)) { //PC4
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