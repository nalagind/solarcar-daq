#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "src/shared/tokens.h"

// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://localhost:8086"
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "solarcar"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "trip"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "EST5EDT,M3.2.0,M11.1.0"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("car");

void setup() {
	Serial.begin(115200);
	
	// Setup wifi
	WiFi.mode(WIFI_STA);
	wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
	
	Serial.print("Connecting to wifi");
	while (wifiMulti.run() != WL_CONNECTED) {
		Serial.print(".");
		delay(100);
	}
	Serial.println();
	
	// Add tags
	sensor.addTag("device", DEVICE);
	sensor.addTag("protocol", "WiFi");
	sensor.addTag("SSID", WiFi.SSID());
	
	// Accurate time is necessary for certificate validation and writing in batches
	// For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
	// Syncing progress and the time will be printed to Serial.
	timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
	
	// Check server connection
	if (client.validateConnection()) {
		Serial.print("Connected to InfluxDB: ");
		Serial.println(client.getServerUrl());
	} else {
		Serial.print("InfluxDB connection failed: ");
		Serial.println(client.getLastErrorMessage());
	}
}

void loop() {
	float battery = 98.0;
	float speed = random(690) / 10.0;
	float temperature = random(420) / 10.0;
	
	// Clear fields for reusing the point. Tags will remain untouched
	sensor.clearFields();
	
	// Store measured value into point
	// Report RSSI of currently connected network
	sensor.addField("rssi", WiFi.RSSI());
	sensor.addField("battery", battery);
	sensor.addField("speed", speed);
	sensor.addField("temperature", temperature);
	
	// Print what are we exactly writing
	Serial.print("Writing: ");
	Serial.println(sensor.toLineProtocol());
	
	// Check WiFi connection and reconnect if needed
	if (wifiMulti.run() != WL_CONNECTED) {
		Serial.println("Wifi connection lost");
	}
	
	// Write point
	if (!client.writePoint(sensor)) {
		Serial.print("InfluxDB write failed: ");
		Serial.println(client.getLastErrorMessage());
	}
	
	delay(1000);
}