//CAN aka TWAI
#include "driver/twai.h"

void setupCANDriver() {
    twai_general_config_t general_config = {
        .mode = TWAI_MODE_NORMAL,
        .tx_io = (gpio_num_t)GPIO_NUM_23,
        .rx_io = (gpio_num_t)GPIO_NUM_22,
        .clkout_io = (gpio_num_t)TWAI_IO_UNUSED,
        .bus_off_io = (gpio_num_t)TWAI_IO_UNUSED,
        .tx_queue_len = 42,
        .rx_queue_len = 69,
        .alerts_enabled = TWAI_ALERT_NONE,
        .clkout_divider = 0
    };
    twai_timing_config_t timing_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t filter_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // esp_err_t error;

    if (twai_driver_install(&general_config, &timing_config, &filter_config) == ESP_OK) {
        Serial.println("CAN Driver installed");
    } else {
        Serial.println("an error occurred installing CAN driver");
        return;
    }

    if (twai_start() == ESP_OK) {
        Serial.println("CAN Driver started");
    } else {
        Serial.println("an error occurred starting CAN driver");
        return;
    }

    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
        Serial.println("CAN Alerts reconfigured");
    } else {
        Serial.println("an error occurred reconfiguring CAN alerts");
        return;
    }
}

void printCANalert(uint32_t& alert, twai_status_info_t& status) {
  if (alert & TWAI_ALERT_ERR_PASS) {
    Serial.println("Alert: TWAI controller has become error passive.");
  }
  if (alert & TWAI_ALERT_BUS_ERROR) {
    Serial.println("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
    Serial.printf("Bus error count: %d\n", status.bus_error_count);
  }
  if (alert & TWAI_ALERT_RX_QUEUE_FULL) {
    Serial.println("Alert: The RX queue is full causing a received frame to be lost.");
    Serial.printf("RX buffered: %d\t", status.msgs_to_rx);
    Serial.printf("RX missed: %d\t", status.rx_missed_count);
    Serial.printf("RX overrun %d\n", status.rx_overrun_count);
  }
}

void printCANmessage(twai_message_t& msg) {
	Serial.print(msg.identifier, HEX);
	Serial.print(" ");
	Serial.print(msg.data_length_code, HEX);
	Serial.print(" ");
	if (!(msg.rtr)) {
		for (int i = 0; i < msg.data_length_code; i++) {
			Serial.print(msg.data[i], HEX);
			Serial.print(" ");
		}
		Serial.println();
	}
}
