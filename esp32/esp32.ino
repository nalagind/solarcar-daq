#define DEVICE "ESP32"
#define FOLDERNAME "/Solar Car Trip Data"
#define FILENAME FOLDERNAME"/data.csv"
#define CAN_DECODE_BASE 10
#define OVERWRITE_DATA

#include "tokens.h"
#include "InfluxdbHelper.h"
#include "CANHelper.h"
#include "SPI.h"
#include "SDhelper.h"
#include <cppQueue.h>

InfluxDBClient client = setupInfluxd();

typedef struct CAN2telemetry {
	twai_message_t message;
	int64_t timestamp;
	unsigned long sn;
} CAN2telemetry;

cppQueue q(sizeof(CAN2telemetry), 111, LIFO, true);
QueueHandle_t loggingQ = xQueueCreate(20, sizeof(EventLogger*));

EventGroupHandle_t wifi_status_event_group;

TaskHandle_t receiveTaskHandle;
TaskHandle_t wifi_tx_hdl;
TaskHandle_t wifi_connect_hdl;
TaskHandle_t server_hdl;
// TaskHandle_t testTaskHandle;

void CANreceive(void* param);
void setup() {
	Serial.begin(115200);

	setupCANDriver();
	
  	wifi_status_event_group = xEventGroupCreate();
	xEventGroupSetBits(wifi_status_event_group, BIT0);

	xTaskCreatePinnedToCore(
		WiFiSend
		, "wifi tx"
		, 10240
		, NULL
		, 2
		, &wifi_tx_hdl
		, 0);
		
	xTaskCreatePinnedToCore(
		simpleServer
		, "server"
		, 10240
		, NULL
		, 1
		, &server_hdl
		, 0);

	xTaskCreatePinnedToCore(
		WiFiConnect
		, "wifi connect"
		, 10240
		, NULL
		, 3
		, &wifi_connect_hdl
		, 0);
	xTaskNotifyGive(wifi_connect_hdl);

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
		,"sd write"
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
	EventLogger *logger;
	
	while (true) {
		xEventGroupWaitBits(wifi_status_event_group, BIT1, pdFALSE, pdFALSE, portMAX_DELAY);
		Serial.println("wifi send started");

		if (client.validateConnection()) {
			Serial.println("Connected to InfluxDB: " + client.getServerUrl());
		} else {
			Serial.println("InfluxDB connection failed: " + client.getLastErrorMessage());
			vTaskSuspend(NULL); // handle exception?
		}

		Point sensor("car");
		sensor.addTag("device", DEVICE);
			
		while (true) {
			logger = NULL;

			if (!q.isEmpty()) {
				// Serial.println("q not empty");
				if (WiFi.status() == WL_CONNECTED) {
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
				} else {
					Serial.println("disconneted, hand over to keep alive");
					xEventGroupClearBits(wifi_status_event_group, BIT1);
					xEventGroupSetBits(wifi_status_event_group, BIT0);
					break;
				}
			} else {
				vTaskDelay(1 / portTICK_PERIOD_MS);
			}
		}
	}
}

void simpleServer(void *param)
{
	String header;
	String redState = "off";
	String greenState = "off";
	String blueState = "off";

	const int rgb_red = 13;
	const int rgb_green = 27;
  	const int rgb_blue = 14;

	unsigned long currentTime = millis();
	unsigned long previousTime = 0;
	const long timeoutTime = 2000;
	pinMode(rgb_red, OUTPUT);
	pinMode(rgb_green, OUTPUT);
	pinMode(rgb_blue, OUTPUT);

	digitalWrite(rgb_red, HIGH);
	digitalWrite(rgb_green, HIGH);
	digitalWrite(rgb_blue, HIGH);

	while (true) {
		xEventGroupWaitBits(wifi_status_event_group, BIT1, pdFALSE, pdFALSE, portMAX_DELAY);
		Serial.println("simple server started");

		Serial.println();
		Serial.println("WiFi connected.");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());

		WiFiServer server(80);
		server.begin();

		while (true) {
			if (WiFi.status() != WL_CONNECTED) {
				Serial.println("disconneted, hand over to keep alive");
				// xTaskNotifyGive(wifi_connect_hdl);
				xEventGroupClearBits(wifi_status_event_group, BIT1);
				xEventGroupSetBits(wifi_status_event_group, BIT0);
				break;
			}

			WiFiClient client = server.available(); // Listen for incoming clients

		if (client) { // If a new client connects
			currentTime = millis();
			previousTime = currentTime;
			Serial.println("New Client");
			String currentLine = "";	   // make a String to hold incoming data from the client
      
			while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
				currentTime = millis();
				if (client.available()) {// if there's bytes to read from the client,
					char c = client.read();
					Serial.write(c);
					header += c;
					if (c == '\n') {
						// if the current line is blank, you got two newline characters in a row.
						// that's the end of the client HTTP request, so send a response:
						if (currentLine.length() == 0) {
							// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
							// and a content-type so the client knows what's coming, then a blank line:
              const char * responseHeader =
              "HTTP/1.1 200 OK\n"
              "Content-type:text/html\n"
              "Connection: close\n"
              "\n";
              client.print(responseHeader);

								// turns the GPIOs on and off
								if (header.indexOf("GET /26/on") >= 0)
								{
									Serial.println("GPIO 26 on");
									redState = "on";
									digitalWrite(rgb_red, HIGH);
								}
								else if (header.indexOf("GET /26/off") >= 0)
								{
									Serial.println("GPIO 26 off");
									redState = "off";
									digitalWrite(rgb_red, LOW);
								}
								else if (header.indexOf("GET /27/on") >= 0)
								{
									Serial.println("GPIO 27 on");
									greenState = "on";
									digitalWrite(rgb_green, HIGH);
								}
								else if (header.indexOf("GET /27/off") >= 0)
								{
									Serial.println("GPIO 27 off");
									greenState = "off";
									digitalWrite(rgb_green, LOW);
								}

								// Display the HTML web page
              const char * html = R"V0G0N(<!DOCTYPE html><html>
              "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
              "<link rel=\"icon\" href=\"data:,\">\n"
								// client.println("<!DOCTYPE html><html>");
								// client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
								// client.println("<link rel=\"icon\" href=\"data:,\">");
								// CSS to style the on/off buttons
								// Feel free to change the background-color and font-size attributes to fit your preferences
              "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n"
              ".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;\n"
              "text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\n"
              ".button2 {background-color: #555555;}</style></head>\n";
              )V0G0N";
								// client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
								// client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
								// client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
								// client.println(".button2 {background-color: #555555;}</style></head>");
              client.println(html);

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

							// Display current state, and ON/OFF buttons for GPIO 27
							client.println("<p>Green - State " + blueState + "</p>");
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
}

void WiFiConnect(void *parameter) {
	while (true) {
		xEventGroupWaitBits(wifi_status_event_group, BIT0, pdTRUE, pdFALSE, portMAX_DELAY);
		
		WiFi.mode(WIFI_STA);
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

		if (WiFi.status() == WL_CONNECTED) {
			xEventGroupSetBits(wifi_status_event_group, BIT1);
			continue;
		}

		Serial.println("connecting to WiFi");
		unsigned long timer = millis();
		while (WiFi.status() != WL_CONNECTED) {
			Serial.println("into wifi connect loop");
			if (millis() - timer < 5000) {
				Serial.print(".");
				vTaskDelay(500 / portTICK_PERIOD_MS);
			} else {
				Serial.println("could not connect to wifi, retry in 5 secs");
				vTaskDelay(5000 / portTICK_PERIOD_MS);
				WiFi.disconnect();
				WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
				Serial.println("connecting to WiFi");
				timer = millis();
			}
		}
			
		// timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
		configTzTime(TZ_INFO, "pool.ntp.org", "time.nis.gov");
    	Serial.print("wifi connected, local IP: ");
		Serial.println(WiFi.localIP());
		xEventGroupSetBits(wifi_status_event_group, BIT1);
	}
}

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
