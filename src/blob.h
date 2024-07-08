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
        char d[7]; snprintf(d, sizeof(d), "%06u", day + month * 100 + year * 10000);
        char t[7]; snprintf(t, sizeof(t), "%06u", sec + min * 100 + hour * 10000);
        l.append(datestamp, d);
        l.append(timestamp, t);
    }
};

struct GPS_Log {
    int satellites;
    float hdop;
    float latitude;
    float longitude;
    int fix_age;
    int altitude_m;
    float speed_kmph;

    void to_csv_string(CSV_Line& l) {
        char s[3]; snprintf(s, sizeof(s), "%2d", satellites);
        char h[5]; dtostrf(hdop, 3, 1, h);
        char lat[12]; dtostrf(latitude, 8, 6, lat);
        char lng[12]; dtostrf(longitude, 8, 6, lng);
        char age[6]; snprintf(age, sizeof(age), "%5d", fix_age);
        char alt[6]; snprintf(alt, sizeof(alt), "%5d", altitude_m);
        char spd[8]; dtostrf(speed_kmph, 5, 2, spd);
        l.append(gps_sat, s);
        l.append(gps_hdop, h);
        l.append(gps_lat, lat);
        l.append(gps_lng, lng);
        l.append(gps_loc_age, age);
        l.append(gps_alt_m, alt);
        l.append(gps_spd_kmph, spd);
    }
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
        l.append(CSV_Header::sn, sn);
        timestamp.to_csv_string(l);
        switch (type) {
            case CAN: {
                process_CAN_msg(can_rx_msg, l);
                break;
            }
            case GPS: {
                gps_log.to_csv_string(l);
                break;
            }
            case DAQ:
            case Radio:
            case Err:
            default: ;
        }
    }
};
