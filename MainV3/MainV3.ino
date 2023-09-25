//Main
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include "CLICommands.h"
#include "CANCommands.h"
#include "SDhelper.h"
#include "LoRaCommands.h"
#define BATCH_SIZE 5
HardwareSerial Serial1(PC5, PB10);
STM32_CAN Can( CAN1, ALT );



// Event group for signaling tasks
EventGroupHandle_t eventGroup;

QueueHandle_t Queue1; // Queue for CAN read data, SD Write n LoRa



// Task handles
TaskHandle_t cliTaskHandle;
TaskHandle_t canReadTaskHandle;
TaskHandle_t canSendTaskHandle;
TaskHandle_t sdWriteTaskHandle;
TaskHandle_t loRaTransmitTaskHandle;
TaskHandle_t loRaReceiveTaskHandle;

// Define event bits
const EventBits_t CLI_EVENT_BIT = (1 << 0);

String batchedData[BATCH_SIZE];
int batchCounter = 0;


void setup() {
	pinMode(PA10, OUTPUT);
	// Set up the SD card
    SPIClass spi;
    setupSD(spi);


    // Initialize serial and USART interrupt
    Serial.begin(115200);
	
	//Initialize CAN & LoRa
	initCAN();
  setupLoRa();


    // Initialize event group
    eventGroup = xEventGroupCreate();

	  Queue1 = xQueueCreate(20, sizeof(String));


    // Initialize CLI and CLI task
	  cli = setupCLI(); 
    xTaskCreate(cliTask, "CLITask", configMINIMAL_STACK_SIZE + 200, NULL, 4, &cliTaskHandle);
    
    // Create tasks based on the selected mode
    if (currentMode == SOLAR_CAR_MODE) {
    // Initialize CAN Read and CAN Send tasks
    xTaskCreate(canReadTask, "CANReadTask", configMINIMAL_STACK_SIZE + 200, NULL, 3, &canReadTaskHandle);
    xTaskCreate(canSendTask, "CANSendTask", configMINIMAL_STACK_SIZE + 200, NULL, 3, &canSendTaskHandle);

    // Initialize SD Write task
    xTaskCreate(sdWriteTask, "SDWriteTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &sdWriteTaskHandle);

    // Create LoRa Transmit and Receive tasks
    xTaskCreate(loRaTransmitTask, "LoRaTransmitTask", configMINIMAL_STACK_SIZE + 200, NULL, 2, &loRaTransmitTaskHandle);

    } else if (currentMode == TRACE_CAR_MODE) {
    // Initialize SD Write task
    xTaskCreate(sdWriteTask, "SDWriteTask", configMINIMAL_STACK_SIZE + 200, NULL, 1, &sdWriteTaskHandle);
    xTaskCreate(loRaReceiveTask, "LoRaReceiveTask", configMINIMAL_STACK_SIZE + 200, NULL, 2, &loRaReceiveTaskHandle);
    }
    // Start the scheduler
    vTaskStartScheduler();
	Serial1.println("Boom!");
	digitalWrite(PA9, HIGH);
}

void loop() {
    // intentionally left empty
}


// CAN Read Task
void canReadTask(void *pvParameters) {
    while (true) {
        CAN_message_t msg;
        if (Can.read(msg)) {
            digitalWrite(PA9, LOW);
            // Process the received message
            String output = processReceivedMessage(msg);
            digitalWrite(PA9, HIGH);
            // Add processed message to batch
            batchedData[batchCounter] = output;
            batchCounter++;

            if (batchCounter >= BATCH_SIZE) {
                // Batch is full, send to Queue1
                xQueueSend(Queue1, batchedData, portMAX_DELAY);
                // Reset batch
                // Reset batch
                for (int i = 0; i < BATCH_SIZE; i++) {
                    batchedData[i] = ""; // Reset to empty string
                }
                batchCounter = 0;
            }
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
  String dataToWrite;
    while (true) {
        if (xQueueReceive(Queue1, &dataToWrite, portMAX_DELAY) == pdTRUE) {
            // Write data to SD card
            appendFile("/data.txt", dataToWrite.c_str());
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
      /*
              if (input == "Solar_Car") {
                  xEventGroupSetBits(eventGroup, (1 << 0)); // Set bit 0 to reset CAN read task
              } else if (input == "Trace_Car") {
                  xEventGroupSetBits(eventGroup, (1 << 1)); // Set bit 1 to reset SD write task
              }**/
              
    }
      vTaskDelay(pdMS_TO_TICKS(10));
    }

void loRaTransmitTask(void *pvParameters) {
    while (true) {
        String batchedData[BATCH_SIZE];
        if (xQueueReceive(Queue1, batchedData, pdMS_TO_TICKS(1000)) == pdTRUE) {
          /*
            // Delta encode the batched data
            /*int compressedData[BATCH_SIZE];
            deltaEncode(batchedData, BATCH_SIZE, compressedData);*/

            // Transmit compressed data via LoRa
            String compressedDataStr;
            for (int i = 0; i < BATCH_SIZE; i++) {
                compressedDataStr += String(batchedData[i]) + ",";
            }
            LoRaTransmit(compressedDataStr);*/
            for (int i = 0; i < BATCH_SIZE; i++) {
                LoRaTransmit(batchedData[i]);
            }
            

        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust the delay as needed
    }
}
void loRaReceiveTask(void *pvParameters) {
    while (true) {
        String ReceivedData = LoRaReceive(); // Implement LoRa receive logic
        xQueueSend(Queue1, ReceivedData, portMAX_DELAY);//send received packed to SDWrite
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust the delay as needed
    }
}

