#include <STM32FreeRTOS.h>
#include "can_helper.h"
#include "FSK_helper.h"
#include "sd_helper.h"
#include "CLICommands.h"
#include "RTC_helper.h"
#define currentMode SOLAR_CAR_MODE
STM32_CAN Can(CAN1, ALT);
static CAN_message_t CAN_RX_msg;
SPIClass SPI_3(PC12, PC11, PC10);
SX1262 radio = new Module(PB3, PA15, PB4, PD2, SPI_3);
SdFat SD;
String can_record;
int record_sn = 1;

SimpleCLI cli = setupCLI();

SemaphoreHandle_t semFSK;
SemaphoreHandle_t semSD;

void setup() {
    Serial.begin(115200);

    // Initialize semaphores
    semFSK = xSemaphoreCreateBinary();
    semSD = xSemaphoreCreateBinary();

    // Check for creation errors
    if (semFSK == NULL || semSD == NULL) {
        Serial.println(F("Creation problem"));
        while (1);
    }

    // Create tasks
    xTaskCreate(canReadTask, "CANReadTask", configMINIMAL_STACK_SIZE + 200, NULL, 2, NULL);
    xTaskCreate(sdWriteTask, "SDWriteTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, NULL);
    xTaskCreate(FSKTransmitTask, "FSKTransmitTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, NULL);

    // Start the scheduler
    vTaskStartScheduler();

    Serial.println("Insufficient RAM");
    while (1);
}

// CAN Read Task
void canReadTask(void *pvParameters) {
    UNUSED(pvParameters);

    while (true) {
        if (Can.read(CAN_RX_msg)) {
            can_record = processReceivedMessage(CAN_RX_msg);
            Serial.println(can_record);

            // Signal the SD writing task to proceed
            xSemaphoreGive(semSD);
            // Signal the FSK transmission task to proceed
            xSemaphoreGive(semFSK);

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// SD Write Task
void sdWriteTask(void *pvParameters) {
    UNUSED(pvParameters);

    while (true) {
        // Wait for the signal from the CAN reading task
        xSemaphoreTake(semSD, portMAX_DELAY);

        // Write to SD card
        if (!writeFile("data.txt", can_record.c_str())) {
            Serial.println("Writing to file failed");
        }

        // Delay for a certain period
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// FSK Transmit Task
void FSKTransmitTask(void *pvParameters) {
    UNUSED(pvParameters);

    while (true) {
        // Wait for the signal from the CAN reading task
        xSemaphoreTake(semFSK, portMAX_DELAY);

        // FSK Transmit
        FSK_Transmit(can_record);

        // Delay for a certain period
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}