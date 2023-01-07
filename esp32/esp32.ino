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
		
	xTaskCreatePinnedToCore(
		simpleServer
		, "server"
		, 10240
		, NULL
		, 1
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

void simpleServer(void *param)
{
	WiFiServer server(80);

	String header;
	String redState = "off";
	String greenState = "off";
	const int output26 = 13;
	const int output27 = 27;
	unsigned long currentTime = millis();
	unsigned long previousTime = 0;
	const long timeoutTime = 2000;
	pinMode(output26, OUTPUT);
	pinMode(output27, OUTPUT);
	// Set outputs to LOW
	digitalWrite(output26, LOW);
	digitalWrite(output27, LOW);
	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	server.begin();

	while (true)
	{
		WiFiClient client = server.available(); // Listen for incoming clients

		if (client)
		{ // If a new client connects,
			currentTime = millis();
			previousTime = currentTime;
			Serial.println("New Client."); // print a message out in the serial port
			String currentLine = "";	   // make a String to hold incoming data from the client
			while (client.connected() && currentTime - previousTime <= timeoutTime)
			{ // loop while the client's connected
				currentTime = millis();
				if (client.available())
				{							// if there's bytes to read from the client,
					char c = client.read(); // read a byte, then
					Serial.write(c);		// print it out the serial monitor
					header += c;
					if (c == '\n')
					{ // if the byte is a newline character
						// if the current line is blank, you got two newline characters in a row.
						// that's the end of the client HTTP request, so send a response:
						if (currentLine.length() == 0)
						{
							// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
							// and a content-type so the client knows what's coming, then a blank line:
							client.println("HTTP/1.1 200 OK");
							client.println("Content-type:text/html");
							client.println("Connection: close");
							client.println();

							// turns the GPIOs on and off
							if (header.indexOf("GET /26/on") >= 0)
							{
								Serial.println("GPIO 26 on");
								redState = "on";
								digitalWrite(output26, HIGH);
							}
							else if (header.indexOf("GET /26/off") >= 0)
							{
								Serial.println("GPIO 26 off");
								redState = "off";
								digitalWrite(output26, LOW);
							}
							else if (header.indexOf("GET /27/on") >= 0)
							{
								Serial.println("GPIO 27 on");
								greenState = "on";
								digitalWrite(output27, HIGH);
							}
							else if (header.indexOf("GET /27/off") >= 0)
							{
								Serial.println("GPIO 27 off");
								greenState = "off";
								digitalWrite(output27, LOW);
							}

							// Display the HTML web page
							client.println("<!DOCTYPE html><html>");
							client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
							client.println("<link rel=\"icon\" href=\"data:,\">");
							// CSS to style the on/off buttons
							// Feel free to change the background-color and font-size attributes to fit your preferences
							client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
							client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
							client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
							client.println(".button2 {background-color: #555555;}</style></head>");

							// Web Page Heading
							client.println("<body><h1>ESP32 Web Server</h1>");

							// Display current state, and ON/OFF buttons for GPIO 26
							client.println("<p>Red - State " + redState + "</p>");
							// If the output26State is off, it displays the ON button
							if (redState == "off")
							{
								client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
							}
							else
							{
								client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
							}

							// Display current state, and ON/OFF buttons for GPIO 27
							client.println("<p>Green - State " + greenState + "</p>");
							// If the output27State is off, it displays the ON button
							if (greenState == "off")
							{
								client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
							}
							else
							{
								client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
							}
							client.println("</body></html>");

							// The HTTP response ends with another blank line
							client.println();
							// Break out of the while loop
							break;
						}
						else
						{ // if you got a newline, then clear currentLine
							currentLine = "";
						}
					}
					else if (c != '\r')
					{					  // if you got anything else but a carriage return character,
						currentLine += c; // add it to the end of the currentLine
					}
					vTaskDelay(1);
				}
				vTaskDelay(1);
			}
			// Clear the header variable
			header = "";
			// Close the connection
			client.stop();
			Serial.println("Client disconnected.");
			Serial.println("");
			vTaskDelay(1);
		}
		
		vTaskDelay(1);
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
