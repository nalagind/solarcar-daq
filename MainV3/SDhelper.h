#pragma once

#include <SD.h>
#include <SPI.h>

bool setupSD(uint32_t miso, uint32_t mosi, uint32_t sclk, uint32_t ssel, const char *path);
bool writeFile(const char * path, const char * message);
void createDir(const char * path);
void appendFile(const char * path, const char * message);
void readFile(const char * path);
