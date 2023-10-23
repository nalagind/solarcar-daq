#ifndef LORA_COMMANDS_H
#define LORA_COMMANDS_H

#include <RadioLib.h>

void setupLoRa();
void LoRaTransmit(String str);
String LoRaReceive();
void deltaEncode(String* data, int length, String* compressedData);
void deltaDecode(String* compressedData, int length, String* decodedData);

#endif // LORA_COMMANDS_H
