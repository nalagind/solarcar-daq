#ifndef SD_HELPER_H
#define SD_HELPER_H

#include "FS.h"
#include "SD.h"
#include <SPI.h>

void setupSD(SPIClass& spi);
void writeFile(fs::FS &fs, const char * path, const char * message);
void createDir(fs::FS &fs, const char * path);
void appendFile(fs::FS &fs, const char * path, const char * message);
void readFile(fs::FS &fs, const char * path);

#endif // SD_HELPER_H
