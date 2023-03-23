//CAN aka TWAI
// #pragma once

#include "driver/twai.h"
#include "logger.h"
#include "preferencesCLI.h"
#define CAN_RX_TASK_NAME "CAN receive"

void setupCANDriver() {
    twai_general_config_t general_config = {
        .mode = can_mode() ? TWAI_MODE_NO_ACK : TWAI_MODE_NORMAL,
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

class CAN_RX_Recorder: public EventLogger {
  private:
  twai_message_t msg;

  public:
  CAN_RX_Recorder(twai_message_t msg, unsigned long sn = 0): EventLogger(0, sn) {
    this->msg = msg;
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
      sprintf(recordLine, "%d", time_ms);
    }
    strcat(recordLine, delimiter);

    // registrar
    strcat(recordLine, CAN_RX_TASK_NAME);
    strcat(recordLine, delimiter);

    // CAN id
    ultoa(msg.identifier, temp, 16);
    strcat(recordLine, temp);
    strcat(recordLine, delimiter);

    // CAN data
    for (int i = 0; i < msg.data_length_code; i++) {
      itoa(msg.data[i], temp, 10);
      strcat(recordLine, temp);
    }
    strcat(recordLine, delimiter);

    // telemetry
    strcat(recordLine, delimiter);

    // source
    strcat(recordLine, delimiter);

    // sn
    ultoa(sn, temp, 10);
    strcat(recordLine, temp);
    strcat(recordLine, delimiter);

    // info
    strcat(recordLine, "random"); // id and data decode

    // finish line
    strcat(recordLine, "\n");
    // Serial.println(recordLine);
    for (int i = 0; i < sizeof(recordLine) / sizeof(char); i++) {
      line[i] = recordLine[i];
    }
  }
};

// static void twai_transmit_task(void *arg)
// {
//     twai_message_t tx_msg = {.data_length_code = 1, .identifier = 0x555, .self = 1};
//     for (int i = 0; i < 3; i++) {
//         xSemaphoreTake(tx_sem, portMAX_DELAY);
//         for (int i = 0; i < 100; i++) {
//             //Transmit messages using self reception request
//             tx_msg.data[0] = i;
//             ESP_ERROR_CHECK(twai_transmit(&tx_msg, portMAX_DELAY));
//             vTaskDelay(pdMS_TO_TICKS(10));
//         }
//     }
//     vTaskDelete(NULL);
// }