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
// QueueHandle_t q;

TaskHandle_t receiveTaskHandle;
// TaskHandle_t testTaskHandle;

void CANreceive(void* param);
void setup() {
	Serial.begin(115200);
  
//   q = xQueueCreate(69, sizeof(CAN2telemetry));
//   if (q == NULL) {
//     Serial.println("Error creating the queue");
//   }

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


	// xTaskCreatePinnedToCore(
	// 	queueTest
	// 	,"q test"
	// 	, 2048
	// 	, NULL
	// 	, 1
	// 	, &testTaskHandle
	// 	, 1);

	xTaskCreatePinnedToCore(
		CANreceive
		,"can rx"
		, 10240
		, NULL
		, 2
		, &receiveTaskHandle
		, 1);
}

void loop() {

}

void WiFiSend(void* param) {
	CAN2telemetry item2send;
	char val[100];
	Point sensor("car");
	sensor.addTag("device", DEVICE);

	while (true) {
		if (!q.isEmpty()) {
			Serial.println("q not empty");
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
				sensor.addField("rssi", WiFi.RSSI());
				sensor.setTime(item2send.timestamp);
				// sensor.setTime(WritePrecision::MS);
				Serial.printf("Writing CAN msg %d\n", item2send.sn);

				if (client.writePoint(sensor)) {
					q.pop(&item2send);
        			Serial.printf("sent %s to db at %d\n", val, millis());
				} else {
					Serial.print("InfluxDB write failed: ");
					Serial.println(client.getLastErrorMessage());
				}
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
//         WiFi.begin("BELL878", "167DA273");

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

	// struct tm tmstruct;
	// struct timeval tv;
	// int64_t time_ms;

	CAN2telemetry transmitQitem;
	unsigned long sn = 1;
	char recordLine[200];
	// char msgId[11];
	// char msgData[9];

	SPIClass spi = SPIClass();
	setupSD(spi);
    createDir(SD, FOLDERNAME);
    writeFile(SD, FILENAME, "time,registrar,CAN id,CAN data,telemetry,source,sn,info\n");
	Serial.println("header written");

	while (true) {
		EventLogger *logger; //get from queue
		recordLine[0] = 0;
		// msgId[0] = 0;
		// msgData[0] = 0;

		twai_read_alerts(&alertsTriggered, portMAX_DELAY);
		twai_get_status_info(&status);
		printCANalert(alertsTriggered, status);

		if (alertsTriggered & TWAI_ALERT_RX_DATA) {
			while (twai_receive(&msg, 0) == ESP_OK) {
				printCANmessage(msg);

				CAN_RX_Recorder rec(msg, sn);
				logger = &rec;
				//above, get from queue
				logger->generateLine(recordLine);


				// if (getLocalTime(&tmstruct)) {
				// 	gettimeofday(&tv, NULL);
				// 	time_ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
				// 	sprintf(recordLine, "%d-%02d-%02d %02d:%02d:%02d:%03d",
				// 		tmstruct.tm_year + 1900 - 2000,
				// 		tmstruct.tm_mon + 1,
				// 		tmstruct.tm_mday,
				// 		tmstruct.tm_hour,
				// 		tmstruct.tm_min,
				// 		tmstruct.tm_sec,
				// 		tv.tv_usec / 1000LL
				// 	);
				// } else {
				// 	time_ms = millis();
				// 	sprintf(recordLine, "%d", time_ms);
				// }
				// strcat(recordLine, ",");

				// // title
				// strcat(recordLine, "crazy fox");
				// strcat(recordLine, ",");

				// ultoa(msg.identifier, msgId, 16);
				// strcat(recordLine, msgId);
				// strcat(recordLine, ",");

				// for (int i = 0; i < msg.data_length_code; i++) {
				// 	itoa(msg.data[i], msgData, 10);
				// 	strcat(recordLine, msgData);
				// }
				// strcat(recordLine, ",");

				// // info
				// strcat(recordLine, "random");
				// strcat(recordLine, "\n");
				Serial.println(recordLine);

				appendFile(SD, FILENAME, recordLine);
				Serial.println();
				readFile(SD, FILENAME);

				transmitQitem = {msg, rec.getTime_ms(), sn++};
				q.push(&transmitQitem);
			}
		}

		// speed++;
		// msgfake.data = speed;
		// // xQueueSendToFront(q, &msg, portMAX_DELAY);
		// // msg = {0,0};
		// // xQueuePeek(q, &msg, portMAX_DELAY);
		// Serial.print("peek at front ");

		// q.push(&msgfake);
		// msg = {0,0};
		// q.peek(&msgfake);
		// Serial.println(msgfake.data);
	}
}

// void queueTest(void* param) {
//   CAN2telemetry msg;
//   Serial.println("did start");

//   while (true) {
// 	if (q.isEmpty()) {
// 		vTaskSuspend(testTaskHandle);
// 	}
// 	msg = {0, 0};
//     // xQueueReceive(q, &msg, portMAX_DELAY);
// 	q.pop(&msg);

// 	Serial.print("receive queue ");
//     Serial.println(msg.data);
//     vTaskDelay(50 / portTICK_PERIOD_MS);
//   }
// }
