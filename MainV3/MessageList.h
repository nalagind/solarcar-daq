#ifndef MESSAGE_LIST_H
#define MESSAGE_LIST_H

#include <map>
#include <Arduino.h>

// Define a map to store message IDs and their names
extern std::map<uint32_t, String> messageNameMap;

// Function to initialize the message name map
void initializeMessageNameMap();

#endif // MESSAGE_LIST_H
