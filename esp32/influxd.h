#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "tokens.h"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "EST5EDT,M3.2.0,M11.1.0"

InfluxDBClient influxdSetup() {
    InfluxDBClient client(
        // InfluxDB client instance with preconfigured InfluxCloud certificate

        "http://localhost:8086", // InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
        "solarcar", // InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
        "trip", // InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
        INFLUXDB_TOKEN,
        InfluxDbCloud2CACert
        
    );
    
    // Accurate time is necessary for certificate validation and writing in batches
	// For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
	// Syncing progress and the time will be printed to Serial.
	timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    return client;
}
