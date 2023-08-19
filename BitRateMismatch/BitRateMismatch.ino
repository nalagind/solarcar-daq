#include <Arduino.h>
#include <RadioLib.h>

// Define the LoRa module and other parameters
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);
const int LoRaDataRate = 20000;  // 20 kbps
const int LoRaTransmissionInterval = 5000;  // Transmit every 5 seconds
const int LoRaTransmissionRate = LoRaDataRate / 8;  // Adjust as needed

// Batching parameters
const int BatchSize = 5;
String batchedData[BatchSize];
int batchedCounter = 0;

// Delta compression function
void deltaEncode(String* data, int length, int* compressedData) {
    compressedData[0] = data[0];  // First string remains unchanged
    for (int i = 1; i < length; i++) {
        String diff;
        for (int j = 0; j < data[i].length(); j++) {
            int diffValue = data[i][j] - data[i - 1][j];
            diff += String(diffValue);
            if (j != data[i].length() - 1) {
                diff += ",";
            }
        }
        compressedData[i] = diff;
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize LoRa module
    radio.begin();
    radio.setFrequency(915e6);
    radio.setSpreadingFactor(7);
    radio.setSignalBandwidth(125000);
    radio.setCodingRate4(5);
    radio.setTxPower(14);
}

void loop() {
    // Simulate reading string data from processed CAN message
    String sensorData = "SensorValue:" + String(random(100));

    // Rate limiting logic
    static unsigned long lastTransmissionTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTransmissionTime >= 1000 / LoRaTransmissionRate) {
        // Batch the data
        batchedData[batchedCounter] = sensorData;
        batchedCounter++;

        if (batchedCounter == BatchSize) {
            // Delta encode the data
            int compressedData[BatchSize];
            deltaEncode(batchedData, BatchSize, compressedData);

            // Transmit compressed data over LoRa
            radio.transmit((byte*)compressedData, BatchSize * sizeof(int));

            // Reset batched data counter
            batchedCounter = 0;
        }

        lastTransmissionTime = currentTime;
    }
}
