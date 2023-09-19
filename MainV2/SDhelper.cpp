#include "SDHelper.h"

void setupSD(SPIClass &spi) {

    SPIClass& SPI_2(PA6,PA7,PC4,PC5);
    SPI_2.begin();
    if (!SD.begin(PB1, spi)) {
        Serial.println("no SD card attached");
        while (true);
    } else {
        Serial.println("SD mounted");
    }
}

void writeFile(const char *path, const char *message) {
    Serial.printf("Writing file: %s\n", path);

    File file =SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void createDir(const char *path) {
    Serial.printf("Creating Dir: %s\n", path);
    if (SD.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void appendFile(const char *path, const char *message) {
    Serial.printf("Appending to file: %s\n", path);

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void readFile(const char *path) {
    Serial.printf("Reading file: %s\n", path);

    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}
