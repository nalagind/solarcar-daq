#pragma once

#include <STM32RTC.h>

extern STM32RTC& rtc;

enum CSV_Header {
    timestamp,
    sn,
    log_type,

    can_ID,
    can_DLC,
    can_remote_request,
    
    can_node_name,

    can_raw_D7,
    can_raw_D6,
    can_raw_D5,
    can_raw_D4,
    can_raw_D3,
    can_raw_D2,
    can_raw_D1,
    can_raw_D0,

    daq_susp_FL_acc_x,
    daq_susp_FL_acc_y,
    daq_susp_FL_acc_z,
    daq_susp_FL_rpm,
    daq_susp_FL_temp,
    
    daq_susp_FR_acc_x,
    daq_susp_FR_acc_y,
    daq_susp_FR_acc_z,
    daq_susp_FR_rpm,
    daq_susp_FR_temp,
    
    daq_susp_RL_acc_x,
    daq_susp_RL_acc_y,
    daq_susp_RL_acc_z,
    daq_susp_RL_rpm,
    daq_susp_RL_temp,
    
    daq_susp_RR_acc_x,
    daq_susp_RR_acc_y,
    daq_susp_RR_acc_z,
    daq_susp_RR_rpm,
    daq_susp_RR_temp,

    comment,

    LAST,
};

const char* csv_header(CSV_Header header) {
    switch(header) {
        case timestamp: return "time";
        case sn: return "sn";
        case log_type: return "log type";
        
        case can_ID: return "can_ID";
        case can_DLC: return "can_DLC";
        case can_node_name: return "can_node_name";
        case can_remote_request: return "can_remote_request";

        case can_raw_D7: return "can_raw_D7";
        case can_raw_D6: return "can_raw_D6";
        case can_raw_D5: return "can_raw_D5";
        case can_raw_D4: return "can_raw_D4";
        case can_raw_D3: return "can_raw_D3";
        case can_raw_D2: return "can_raw_D2";
        case can_raw_D1: return "can_raw_D1";
        case can_raw_D0: return "can_raw_D0";

        case daq_susp_FL_acc_x: return "daq_susp_FL_acc_x";
        case daq_susp_FL_acc_y: return "daq_susp_FL_acc_y";
        case daq_susp_FL_acc_z: return "daq_susp_FL_acc_z";
        case daq_susp_FL_rpm: return "daq_susp_FL_rpm";
        case daq_susp_FL_temp: return "daq_susp_FL_temp";

        case daq_susp_FR_acc_x: return "daq_susp_FR_acc_x";
        case daq_susp_FR_acc_y: return "daq_susp_FR_acc_y";
        case daq_susp_FR_acc_z: return "daq_susp_FR_acc_z";
        case daq_susp_FR_rpm: return "daq_susp_FR_rpm";
        case daq_susp_FR_temp: return "daq_susp_FR_temp";

        case daq_susp_RL_acc_x: return "daq_susp_RL_acc_x";
        case daq_susp_RL_acc_y: return "daq_susp_RL_acc_y";
        case daq_susp_RL_acc_z: return "daq_susp_RL_acc_z";
        case daq_susp_RL_rpm: return "daq_susp_RL_rpm";
        case daq_susp_RL_temp: return "daq_susp_RL_temp";

        case daq_susp_RR_acc_x: return "daq_susp_RR_acc_x";
        case daq_susp_RR_acc_y: return "daq_susp_RR_acc_y";
        case daq_susp_RR_acc_z: return "daq_susp_RR_acc_z";
        case daq_susp_RR_rpm: return "daq_susp_RR_rpm";
        case daq_susp_RR_temp: return "daq_susp_RR_temp";

        case comment: return "comment";

        case LAST: return "";
    }
}

struct Log {
    int front = 0;
    int indices[10];
    char* values[10];

    void append(CSV_Header header, int value) {
        indices[front] = header;
        char val[7];
        values[front] = itoa(value, val, 10);
        front++;
    }

    void append(CSV_Header header, uint8_t value) {
        indices[front] = header;
        char val[4];
        values[front] = itoa(value, val, 10);
        front++;
    }

    void append(CSV_Header header, uint32_t value) {
        indices[front] = header;
        char val[11];
        values[front] = itoa(value, val, 10);
        front++;
    }

    void append(CSV_Header header, float value) {
        indices[front] = header;
        char val[32];
        // snprintf(val, sizeof(val), "%.3f", value);
        dtostrf(value, 6, 3, val);
        values[front] = val;
        front++;

        for (int i = 0; i < front; i++) {
            Serial.println(values[i]);
        }
    }

    void append(CSV_Header header, char* value) {
        indices[front] = header;
        values[front] = value;
        front++;
    }

    Log(uint32_t sn, char* log_type) {
        indices[front] = CSV_Header::timestamp;
        char val[20];
        snprintf(val, sizeof(val), "%04d/%02d/%02d %02d:%02d:%02d", rtc.getYear() + 2000, rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
        values[front] = val;
        front++;

        indices[front] = CSV_Header::sn;
        values[front] = itoa(sn, val, 10);
        front++;

        indices[front] = CSV_Header::log_type;
        values[front] = log_type;
        front++;
    }

    void generate_row(char* buffer) {
        strcpy(buffer, "");
        int last_index = 0;
        for (int i = 0; i < front; i++) {
            for (int j = last_index; j < indices[i]; j++) {
                strcat(buffer, ",");
            }
            last_index = indices[i];
            strcat(buffer, values[i]);
        }
        for (int i = last_index; i < CSV_Header::LAST - 1; i++) {
            strcat(buffer, ",");
        }
        strcat(buffer, "\r\n");
    }

    void describe(char* buffer, CSV_Header headers_req[], int headers_req_count) {
        strcpy(buffer, "");
        for (int i = 0; i < front; i++) {
            for (int j = 0; j < headers_req_count; j++) {
                if (headers_req[j] == indices[i]) {
                    buffer += sprintf(buffer, "%s %s\r\n", csv_header(headers_req[j]), values[i]);
                    break;
                }
            }
        }
    }
};