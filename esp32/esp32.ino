#define DEVICE "ESP32"
#define CAN_DECODE_BASE 10
#define OVERWRITE_DATA
#define LOG_TAG "DAQ"

#include "tokens.h"
#include "InfluxdbHelper.h"
#include "CANHelper.h"
#include "SPI.h"
#include "SDhelper.h"
#include "preferencesCLI.h"
#include <cppQueue.h>

typedef struct CAN2telemetry {
	twai_message_t message;
	int64_t timestamp;
	unsigned long sn;
} CAN2telemetry;

cppQueue q(sizeof(CAN2telemetry), 111, LIFO, true);
QueueHandle_t loggingQ = xQueueCreate(50, sizeof(EventLogger*));

EventGroupHandle_t status_event_group;

TaskHandle_t receiveTaskHandle;
TaskHandle_t wifi_tx_hdl;
TaskHandle_t wifi_connect_hdl;
TaskHandle_t server_hdl;
TaskHandle_t sd_hdl;
TaskHandle_t can_tx_hdl;

SemaphoreHandle_t rx_sem;
SemaphoreHandle_t tx_sem;
SemaphoreHandle_t ctrl_sem;

bool WIFI_SET = false;
bool INFLUX_SET = false;
bool CAN_SET = false;

void IRAM_ATTR sd_detect_isr() {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (digitalRead(21) == LOW) {
		vTaskNotifyGiveFromISR(sd_hdl, &xHigherPriorityTaskWoken);
	}
}

void setup() {
	Serial.begin(115200);
	
	status_event_group = xEventGroupCreate();
    tx_sem = xSemaphoreCreateBinary();
	rx_sem = xSemaphoreCreateBinary();
	ctrl_sem = xSemaphoreCreateBinary();

	attachInterrupt(21, sd_detect_isr, CHANGE);

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

	xTaskCreatePinnedToCore(
		CANreceive
		, "can rx"
		, 10240
		, NULL
		, 3
		, &receiveTaskHandle
		, 1);

	xTaskCreatePinnedToCore(
		CANcontrol
		, "can control"
		, 10240
		, NULL
		, 5
		, NULL
		, 1);
		
	xTaskCreatePinnedToCore(
		SDwrite
		, "sd write"
		, 10240
		, NULL
		, 2
		, &sd_hdl
		, 1);

	xTaskCreate(
		readConfig
		, "cli"
		, 10240
		, NULL
		, 1
		, NULL);
}

void loop() {

}

void readConfig(void* arg) {
	SimpleCLI cli = setupCLI();

	while (true) {
		while (!Serial.available()) {}
		
		String input = Serial.readStringUntil('\n');
		Serial.print("% ");
		Serial.println(input);
		cli.parse(input);

		if (WIFI_SET) {
			Serial.println("wifi set");
			xEventGroupClearBits(status_event_group, BIT1);
			xEventGroupSetBits(status_event_group, BIT0);
			WIFI_SET = false;
		} else if (INFLUX_SET) {
			Serial.println("influx set");
			xEventGroupSetBits(status_event_group, BIT2);
			INFLUX_SET = false;
		} else if (CAN_SET) {
			Serial.println("can mode set");
			xEventGroupSetBits(status_event_group, BIT3);
			CAN_SET = false;
		}
	}
}

void WiFiSend(void* param) {
	CAN2telemetry item2send;
	char val[100];
	EventLogger *logger;
	
	while (true) {
		xEventGroupWaitBits(status_event_group, BIT1, pdFALSE, pdFALSE, portMAX_DELAY);
		// Serial.println("wifi send started");

		InfluxDBClient client = setupInfluxd();
		if (client.validateConnection()) {
			Serial.println("Connected to InfluxDB: " + client.getServerUrl());
		} else {
			// Serial.println("InfluxDB connection failed: " + client.getLastErrorMessage());
			xEventGroupSetBits(status_event_group, BIT1);
			vTaskDelay(3000);
			continue;
		}
		Point sensor("car");
		sensor.addTag("device", DEVICE);
			
		while (true) {
			logger = NULL;

			EventBits_t uxBits = xEventGroupWaitBits(status_event_group, BIT2, pdTRUE, pdFALSE, 0);
			if ((uxBits & BIT2) == BIT2) {
				Serial.println("config set, restart influx task");
				break;
			}

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
					xEventGroupClearBits(status_event_group, BIT1);
					xEventGroupSetBits(status_event_group, BIT0);
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
		xEventGroupWaitBits(status_event_group, BIT1, pdFALSE, pdFALSE, portMAX_DELAY);
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
				xEventGroupClearBits(status_event_group, BIT1);
				xEventGroupSetBits(status_event_group, BIT0);
				break;
			}

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
								if (header.indexOf("GET /13/on") >= 0)
								{
									Serial.println("GPIO 26 on");
									redState = "on";
									digitalWrite(rgb_red, LOW);
								}
								else if (header.indexOf("GET /13/off") >= 0)
								{
									Serial.println("GPIO 26 off");
									redState = "off";
									digitalWrite(rgb_red, HIGH);
								}
								else if (header.indexOf("GET /27/on") >= 0)
								{
									Serial.println("GPIO 27 on");
									greenState = "on";
									digitalWrite(rgb_green, LOW);
								}
								else if (header.indexOf("GET /27/off") >= 0)
								{
									Serial.println("GPIO 27 off");
									greenState = "off";
									digitalWrite(rgb_green, HIGH);
								}
								else if (header.indexOf("GET /14/on") >= 0)
								{
									Serial.println("GPIO 14 on");
									blueState = "on";
									digitalWrite(rgb_blue, LOW);
								}
								else if (header.indexOf("GET /14/off") >= 0)
								{
									Serial.println("GPIO 14 off");
									blueState = "off";
									digitalWrite(rgb_blue, HIGH);
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

								// Display current state, and ON/OFF buttons for GPIO 13
								client.println("<p>Red - State " + redState + "</p>");
								// If the output13State is off, it displays the ON button
								if (redState == "off")
								{
									client.println("<p><a href=\"/13/on\"><button class=\"button\">OFF</button></a></p>");
								}
								else
								{
									client.println("<p><a href=\"/13/off\"><button class=\"button button2\">ON</button></a></p>");
								}

								// Display current state, and ON/OFF buttons for GPIO 27
								client.println("<p>Green - State " + greenState + "</p>");
								// If the output27State is off, it displays the ON button
								if (greenState == "off")
								{
									client.println("<p><a href=\"/27/on\"><button class=\"button\">OFF</button></a></p>");
								}
								else
								{
									client.println("<p><a href=\"/27/off\"><button class=\"button button2\">ON</button></a></p>");
								}

								// Display current state, and ON/OFF buttons for GPIO 14
								client.println("<p>Blue - State " + blueState + "</p>");
								// If the output14State is off, it displays the ON button
								if (blueState == "off")
								{
									client.println("<p><a href=\"/14/on\"><button class=\"button\">OFF</button></a></p>");
								}
								else
								{
									client.println("<p><a href=\"/14/off\"><button class=\"button button2\">ON</button></a></p>");
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
	xEventGroupSetBits(status_event_group, BIT0);
	while (true) {
		xEventGroupWaitBits(status_event_group, BIT0, pdTRUE, pdFALSE, portMAX_DELAY);
		
		WiFi.mode(WIFI_STA);
		WiFi.begin(wifi_SSID().c_str(), wifi_password().c_str());

		Serial.println("WiFi connecting");
		unsigned long timer = millis();
		while (WiFi.status() != WL_CONNECTED && millis() - timer < 4200) {
			Serial.print(".");
			vTaskDelay(420 / portTICK_PERIOD_MS);
		}

		if (WiFi.status() == WL_CONNECTED) {
			// timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
			configTzTime(TZ_INFO, "pool.ntp.org", "time.nis.gov");
			Serial.print("Connected, local IP: ");
			Serial.println(WiFi.localIP());
			xEventGroupSetBits(status_event_group, BIT1);
		} else {
			Serial.println("could not connect, retry in 5 secs");
			WiFi.disconnect();
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			xEventGroupSetBits(status_event_group, BIT0);
		}
	}
}

void CANreceive(void* param) {
	uint32_t alertsTriggered;
	twai_status_info_t status;
	twai_message_t msg;

	CAN2telemetry transmitQitem;
	unsigned long sn = 1;

	EventLogger *logger;

	xSemaphoreTake(rx_sem, portMAX_DELAY);
	int count;
	Serial.println("can rx started");
	
	while (true) {
		logger = NULL;

		twai_read_alerts(&alertsTriggered, portMAX_DELAY);
		twai_get_status_info(&status);
		printCANalert(alertsTriggered, status);

		if (alertsTriggered & TWAI_ALERT_RX_DATA) {
			while (twai_receive(&msg, 0) == ESP_OK) {
				printCANmessage(msg);
				// Serial.printf("can received %d\n", ++count);

				logger = new CAN_RX_Recorder(msg, sn);
				xQueueSend(loggingQ, &logger, portMAX_DELAY);
				transmitQitem = {msg, logger->getTime_ms(), sn++};
				q.push(&transmitQitem);
			}
		}
	}
}

void CANcontrol(void *arg) {
	while (true) {
		setupCANDriver();
		// ESP_LOGI(LOG_TAG, "driver started");
		Serial.println("driver started");

		xSemaphoreGive(rx_sem);
		if (can_is_loopback()) {
			xTaskCreatePinnedToCore(
				twai_transmit_task
				, "can tx"
				, 10240
				, NULL
				, 4
				, &can_tx_hdl
				, 1);

			while (true) {
				xSemaphoreGive(tx_sem);
				// ESP_LOGI(LOG_TAG, "loopback tx task started");
				Serial.println("loopback tx task started");
				xSemaphoreTake(ctrl_sem, portMAX_DELAY);
				EventBits_t uxBits = xEventGroupWaitBits(status_event_group, BIT3, pdTRUE, pdFALSE, 0);
				if ((uxBits & BIT3) == BIT3) {
					vTaskDelete(can_tx_hdl);
					break;
				}
				vTaskDelay(1000);
			}
		} else {
			xEventGroupWaitBits(status_event_group, BIT3, pdTRUE, pdFALSE, portMAX_DELAY);
		}
		
		Serial.println("config set, restart can");
		ESP_ERROR_CHECK(twai_stop());
		ESP_ERROR_CHECK(twai_driver_uninstall());
	}
}

void SDwrite(void* param) {
	SPIClass spi = SPIClass();
	if (digitalRead(21) == LOW) {
		xTaskNotifyGive(sd_hdl);
	}

	while (true) {
		if (digitalRead(21) == LOW) {
			if (ulTaskNotifyTake(pdTRUE, 0) != 0) {
				if (!setupSD(spi)) {
					xTaskNotifyGive(sd_hdl);
					delay(1000);
				}
			}
		} else {
			uint32_t ntft = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			Serial.println(ntft);
			Serial.print("SD attached...");
			if (!setupSD(spi)) {
				xTaskNotifyGive(sd_hdl);
				delay(1000);
			}
		}

		EventLogger *logger;
		char recordLine[200];

		logger = NULL;
		recordLine[0] = 0;

		xQueueReceive(loggingQ, &logger, portMAX_DELAY);
		if (logger == NULL) { continue; }
		logger->generateLine(recordLine);

		if (digitalRead(21) == HIGH) {
			Serial.println("SD detached");
			delete logger;
			continue;
		}
		delay(50);
		appendFile(SD, sd_filename().c_str(), recordLine);
		Serial.println();

		delete logger;
	}
}
