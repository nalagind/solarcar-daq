//Main
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "CLICommands.h"
#include "CANCommands.h"
#include "SDhelper.h"


HardwareSerial Serial1(PC5, PB10);
STM32_CAN Can( CAN1, ALT );
HardwareSerial Serial1(PC5, PB10);
static CAN_message_t CAN_TX_msg;
SimpleCLI cli;

// Event group for signaling tasks
EventGroupHandle_t eventGroup;

QueueHandle_t canReadQueue; // Queue for CAN read data
QueueHandle_t sdWriteQueue; // Queue for SD write data


// Task handles
TaskHandle_t cliTaskHandle;
TaskHandle_t canReadTaskHandle;
TaskHandle_t canSendTaskHandle;
TaskHandle_t sdWriteTaskHandle;

// Define event bits
const EventBits_t CLI_EVENT_BIT = (1 << 0);


// CAN Read Task
void canReadTask(void *pvParameters) {
    while (true) {
        CAN_message_t msg;
        if (Can.read(msg)) {
            digitalWrite(PA9, LOW);
            // Process the received message
            String output = processReceivedMessage(msg);
            digitalWrite(PA9, HIGH);
            xQueueSend(sdWriteQueue, &output, portMAX_DELAY); // Send data to SD write task
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// CAN Send Task
void canSendTask(void *pvParameters) {
    while (true) {
        // Implement CAN send logic here
        vTaskDelay(pdMS_TO_TICKS(1000)); // Example delay
    }
}

// SD Write Task
void sdWriteTask(void *pvParameters) {
    while (true) {
        if (xQueueReceive(sdWriteQueue, &dataToWrite, portMAX_DELAY) == pdTRUE) {
            // Write data to SD card
            appendFile(SD, "/data.txt", dataToWrite.c_str());
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust the delay as needed
    }
}

// CLI Task
void cliTask(void *pvParameters) {
    while (true) {
        // Process CLI input and commands
		while (!Serial.available()) {}
		
		String input = Serial.readStringUntil('\n');
		Serial.print("% ");
		input.trim();
		Serial.println(input);
		cli.parse(input);
		
            if (input == "reset-can-read") {
                xEventGroupSetBits(eventGroup, (1 << 0)); // Set bit 0 to reset CAN read task
            } else if (input == "reset-sd-write") {
                xEventGroupSetBits(eventGroup, (1 << 1)); // Set bit 1 to reset SD write task
            }
            
            cli.parse(input);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
	pinMode(PA10, OUTPUT);
	// Set up the SD card
    SPIClass spi;
    setupSD(spi);
    // Initialize serial and USART interrupt
    Serial.begin(115200);
	
	//Initialize CAN
	initCAN()

    // Initialize event group
    eventGroup = xEventGroupCreate();

	canReadQueue = xQueueCreate(10, sizeof(CAN_message_t)); // Create CAN read queue
    sdWriteQueue = xQueueCreate(10, sizeof(String)); // Create SD write queue


    // Initialize CLI and CLI task
	cli = setupCLI(); 
    setupCLICommands();
    xTaskCreate(cliTask, "CLITask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &cliTaskHandle);

    // Initialize CAN Read and CAN Send tasks
    xTaskCreate(canReadTask, "CANReadTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &canReadTaskHandle);
    xTaskCreate(canSendTask, "CANSendTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &canSendTaskHandle);

    // Initialize SD Write task
    xTaskCreate(sdWriteTask, "SDWriteTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &sdWriteTaskHandle);

    // Start the scheduler
    vTaskStartScheduler();
	Serial1.println("Boom!");
	digitalWrite(PA9, HIGH);
}

void loop() {
    // intentionally left empty
}
