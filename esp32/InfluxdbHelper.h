#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "tokens.h"
#include "logger.h"
#define WiFi_TX_TASK_NAME "WiFi tx"
#define INFLUXDB_BUCKET "drive"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "EST5EDT,M3.2.0,M11.1.0"

InfluxDBClient setupInfluxd() {
    InfluxDBClient client (
        // InfluxDB client instance with preconfigured InfluxCloud certificate

        INFLUXDB_URL, // InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
        "solarcar", // InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
        INFLUXDB_BUCKET, // InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
        INFLUXDB_TOKEN,
        InfluxDbCloud2CACert
        
    );
    
    client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::MS)); // Set write precision to milliseconds. Leave other parameters default.

    // BucketsClient buckets = client.getBucketsClient();
    // if (!buckets.checkBucketExists(INFLUXDB_BUCKET)) {
    //   Bucket b = buckets.createBucket(INFLUXDB_BUCKET, 0); // retention policy in sec, 0 = infinity
    //   if (!b) {
    //     Serial.print("could not create bucket: ");
    //     Serial.println(buckets.getLastErrorMessage());
    //     while (true) {};
    //   }
    //   Serial.printf("created bucket: %s\n", b.toString());
    // }

    return client;
}

class WiFi_TX_Logger: public EventLogger {
  private:
  int8_t rssi;
  const char* result;

  public:
  WiFi_TX_Logger(const char *dataSourceTaskName, unsigned long sn, int8_t rssi, const char* result = 0): EventLogger(dataSourceTaskName, sn) {
    this->rssi = rssi;
    this->result = result;
  }

  void generateLine(char *line, const char *delimiter = ",") {
    char recordLine[200];
    char temp[11];
    recordLine[0] = 0;
    temp[0] = 0;

    // time
		if (realtime) {
      sprintf(recordLine, "%d-%02d-%02d %02d:%02d:%02d:%03d",
        tmstruct.tm_year + 1900 - 2000,
        tmstruct.tm_mon + 1,
        tmstruct.tm_mday,
        tmstruct.tm_hour,
        tmstruct.tm_min,
        tmstruct.tm_sec,
        tv.tv_usec / 1000LL);
    } else {
      sprintf(recordLine, "%d (ms)", time_ms);
    }
    strcat(recordLine, delimiter);

    // registrar
    strcat(recordLine, WiFi_TX_TASK_NAME);
    strcat(recordLine, delimiter);

    // CAN id
    strcat(recordLine, delimiter);
    
    // CAN data
    strcat(recordLine, delimiter);

    // telemetry
    itoa(rssi, temp, 10);
    strcat(recordLine, temp);
    strcat(recordLine, delimiter);
    
    // source
    strcat(recordLine, dataSourceTaskName);
    strcat(recordLine, delimiter);

    // sn
    ultoa(sn, temp, 10);
    strcat(recordLine, temp);
    strcat(recordLine, delimiter);

    // info
    strcat(recordLine, result);
    
    // finish line
    strcat(recordLine, "\n");
    // Serial.println(recordLine);
    for (int i = 0; i < sizeof(recordLine) / sizeof(char); i++) {
      line[i] = recordLine[i];
    }
  }
};
