#include "SDHelper.h"

bool sd_init(uint32_t miso, uint32_t mosi, uint32_t sclk, uint32_t ssel, const char *path) {
    //bad library -> #todo: SdFat https://github.com/greiman/SdFat

    SPI.setMISO(miso);
    SPI.setMOSI(mosi);
    SPI.setSCLK(sclk);

    if (!SD.begin(ssel)) {
        Serial.println("...but mounting failed");
        return false;
    }

    if (!writeFile(path, "time,registrar,CAN id,CAN data,telemetry,source,sn,info\n")) return false;
    Serial.println("...and mounted");
    return true;
}

bool writeFile(const char *path, const char *message) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
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

void createDir(const char *path) {
    Serial.println("Creating Dir...\n");
    if (SD.mkdir(path)) {
        Serial.println("...success!");
    } else {
        Serial.println("...failed");
    }
}

// void appendFile(const char *path, const char *message) {
//     Serial.printf("Appending to file: %s\n", path);

//     File file = SD.open(path, FILE_APPEND);
//     if (!file) {
//         Serial.println("Failed to open file for appending");
//         return;
//     }
//     if (file.print(message)) {
//         Serial.println("Message appended");
//     } else {
//         Serial.println("Append failed");
//     }
//     file.close();
// }

// void readFile(const char *path) {
//     Serial.printf("Reading file: %s\n", path);

//     File file = SD.open(path);
//     if (!file) {
//         Serial.println("Failed to open file for reading");
//         return;
//     }

//     Serial.print("Read from file: ");
//     while (file.available()) {
//         Serial.write(file.read());
//     }
//     file.close();
// }
