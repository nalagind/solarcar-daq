#ifndef SD_HELPER_H
#define SD_HELPER_H


#include "SD.h"
#include <SPI.h>

void setupSD(SPIClass& spi);
void writeFile(const char * path, const char * message);
void createDir(const char * path);
void appendFile(const char * path, const char * message);
void readFile(const char * path);

#endif // SD_HELPER_H
