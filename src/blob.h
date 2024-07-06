#pragma once

#include "csv_logger.h"
#include "STM32_CAN.h"
#include <STM32RTC.h>

extern STM32RTC& rtc;
uint32_t log_sn = 1;

enum LogType {
    CAN, GPS, DAQ, Radio, Err
};

struct Timestamp {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    Timestamp() {
        year = rtc.getYear();
        month = rtc.getMonth();
        day = rtc.getDay();
        hour = rtc.getHours();
        min = rtc.getMinutes();
        sec = rtc.getSeconds();
    }

    void to_csv_string(CSV_Line& l) {
        char d[7];
        snprintf(d, sizeof(d), "%06u", day + month * 100 + year * 10000);
        char t[7];
        snprintf(t, sizeof(t), "%06u", sec + min * 100 + hour * 10000);
        l.append(datestamp, d);
        l.append(timestamp, t);
    }
};

struct GPS_Log {
    float latitude;
    float longitude;
    int fix_age;
    int altitude_m;
    int speed_kmph;
    int satellites;
};

struct LogBlob {
    LogType type;
    Timestamp timestamp;
    uint32_t sn;
    union {
        GPS_Log gps_log;
        CAN_message_t can_rx_msg;
    };

    LogBlob(LogType t, uint32_t& sn = log_sn): type{t}, sn{sn++} {};

    void to_csv_line(CSV_Line& l) {
        l.append(log_type, type);
        timestamp.to_csv_string(l);
        switch (type) {
            case CAN: {
                // process_CAN_msg(can_rx_msg, l);
                break;
            }
            case GPS:
            case DAQ:
            case Radio:
            case Err:
            default: ;
        }
    }
};
