#pragma once

bool sd_init(uint32_t chipSelectPin, uint32_t miso, uint32_t mosi, uint32_t sclk, const char *path);
bool writeFile(const char * path, const char * message);
void createDir(const char * path);
void appendFile(const char * path, const char * message);
void readFile(const char * path);
