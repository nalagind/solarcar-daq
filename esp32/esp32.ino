#define DEVICE "ESP32"
#define FOLDERNAME "/Solar Car Trip Data"
#define FILENAME FOLDERNAME"/data.csv"
#define CAN_DECODE_BASE 10
#define OVERWRITE_DATA

#include <WiFiMulti.h>
#include "tokens.h"
#include "InfluxdbHelper.h"
#include "CANHelper.h"
#include "SPI.h"
#include "SDhelper.h"
#include <cppQueue.h>

WiFiMulti wifiMulti;
InfluxDBClient client = setupInfluxd();

typedef struct CAN2telemetry {
	twai_message_t message;
	int64_t timestamp;
	unsigned long sn;
} CAN2telemetry;

cppQueue q(sizeof(CAN2telemetry), 111, LIFO, true);
QueueHandle_t loggingQ = xQueueCreate(20, sizeof(EventLogger*));

TaskHandle_t receiveTaskHandle;
// TaskHandle_t testTaskHandle;

void CANreceive(void* param);
void setup() {
	Serial.begin(115200);

	setupCANDriver();

	WiFi.mode(WIFI_STA);
	wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
	Serial.print("Connecting to wifi");
	while (wifiMulti.run() != WL_CONNECTED) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();
	
	// Accurate time is necessary for certificate validation and writing in batches
	// For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
	// Syncing progress and the time will be printed to Serial.
	timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
	
	if (client.validateConnection()) {
		Serial.print("Connected to InfluxDB: ");
		Serial.println(client.getServerUrl());
	} else {
		Serial.print("InfluxDB connection failed: ");
		Serial.println(client.getLastErrorMessage());
		while(true);
	}

	xTaskCreatePinnedToCore(
		WiFiSend
		, "WiFi TX"
		, 10240
		, NULL
		, 2
		, NULL
		, 0);

//  xTaskCreatePinnedToCore(
//	keepWiFiAlive,
//	"keepWiFiAlive",  // Task name
//	5000,             // Stack size (bytes)
//	NULL,             // Parameter
//	1,                // Task priority
//	NULL,             // Task handle
//	ARDUINO_RUNNING_CORE
//);

	xTaskCreatePinnedToCore(
		CANreceive
		,"can rx"
		, 10240
		, NULL
		, 3
		, &receiveTaskHandle
		, 1);
		
	xTaskCreatePinnedToCore(
		SDwrite
		,"SD write"
		, 10240
		, NULL
		, 2
		, NULL
		, 1);
}

void loop() {

}

void WiFiSend(void* param) {
	CAN2telemetry item2send;
	char val[100];
	Point sensor("car");
	sensor.addTag("device", DEVICE);
	EventLogger *logger;

	while (true) {
		logger = NULL;

		if (!q.isEmpty()) {
			// Serial.println("q not empty");
			if (wifiMulti.run() == WL_CONNECTED) {
				q.peek(&item2send);
				sensor.clearFields();
				val[0] = 0;

				for (int i = 0; i < item2send.message.data_length_code; i++) {
					char data[9];
					itoa(item2send.message.data[i], data, 10);
					strcat(val, data);
				}
				sensor.addField("billboard", atoi(val));
				int8_t rssi = WiFi.RSSI();
				sensor.addField("rssi", rssi);
				sensor.setTime(item2send.timestamp);
				// sensor.setTime(WritePrecision::MS);
				Serial.printf("Writing CAN msg %d\n", item2send.sn);

				if (client.writePoint(sensor)) {
					q.pop(&item2send);
        			Serial.printf("sent %s to db at %d\n", val, millis());
					logger = new WiFi_TX_Logger(CAN_RX_TASK_NAME, item2send.sn, rssi, "success");
				} else {
					Serial.print("InfluxDB write failed: ");
					String err = client.getLastErrorMessage();
					int strlen = err.length() + 1;
					char temp[strlen];
					err.toCharArray(temp, strlen);
					logger = new WiFi_TX_Logger(CAN_RX_TASK_NAME, item2send.sn, rssi, temp);
					Serial.println(err);
				}
				
				xQueueSend(loggingQ, &logger, portMAX_DELAY);
				vTaskDelay(1 / portTICK_PERIOD_MS);
			}
			else {
				// unsigned long timer = millis();

				// while (wifiMulti.run() != WL_CONNECTED && (millis() - timer) < 1000) {}
				// if (wifiMulti.run() == WL_CONNECTED) {
				// 	timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
				// } else {
				// 	Serial.println("couldn't connect to wifi");
				// 	vTaskDelay(3000 / portTICK_PERIOD_MS);
				// }
			}
		} else {
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
	}

	

}

// void keepWiFiAlive(void * parameter){
//     for(;;){
//         if(WiFi.status() == WL_CONNECTED){
//             vTaskDelay(10000 / portTICK_PERIOD_MS);
//             continue;
//         }

//         Serial.println("[WIFI] Connecting");
//         WiFi.mode(WIFI_STA);
//         WiFi.begin("", "");

//         unsigned long startAttemptTime = millis();

//         // Keep looping while we're not connected and haven't reached the timeout
//         while (WiFi.status() != WL_CONNECTED && 
//                 millis() - startAttemptTime < 1000){}

//         // When we couldn't make a WiFi connection (or the timeout expired)
// 		  // sleep for a while and then retry.
//         if(WiFi.status() != WL_CONNECTED){
//             Serial.println("[WIFI] FAILED");
//             vTaskDelay(3000 / portTICK_PERIOD_MS);
// 			  continue;
//         }

//         Serial.println("[WIFI] Connected: " + WiFi.localIP());
//     }
// }

void CANreceive(void* param) {
	uint32_t alertsTriggered;
	twai_status_info_t status;
	twai_message_t msg;

	CAN2telemetry transmitQitem;
	unsigned long sn = 1;

	EventLogger *logger;

	while (true) {
		logger = NULL;

		twai_read_alerts(&alertsTriggered, portMAX_DELAY);
		twai_get_status_info(&status);
		printCANalert(alertsTriggered, status);

		if (alertsTriggered & TWAI_ALERT_RX_DATA) {
			while (twai_receive(&msg, 0) == ESP_OK) {
				printCANmessage(msg);

				logger = new CAN_RX_Recorder(msg, sn);
				xQueueSend(loggingQ, &logger, portMAX_DELAY);
				transmitQitem = {msg, logger->getTime_ms(), sn++};
				q.push(&transmitQitem);
			}
		}
	}
}

void SDwrite(void* param) {
	SPIClass spi = SPIClass();
	setupSD(spi);
    createDir(SD, FOLDERNAME);
    writeFile(SD, FILENAME, "time,registrar,CAN id,CAN data,telemetry,source,sn,info\n");
	Serial.println("header written");

	EventLogger *logger;
	char recordLine[200];

	while (true) {
		logger = NULL;
		recordLine[0] = 0;

		xQueueReceive(loggingQ, &logger, portMAX_DELAY);
		if (logger == NULL) { continue; }
		logger->generateLine(recordLine);

		appendFile(SD, FILENAME, recordLine);
		Serial.println();
		readFile(SD, FILENAME);
		Serial.println();

		delete logger;
	}
}
