#pragma once

#include "csv_logger.h"
#include "STM32_CAN.h"
#include <STM32RTC.h>
#include "sd_helper.h"

extern STM32RTC& rtc;
uint32_t log_sn = 1;

enum LogType {
    CAN, GPS, DAQ, Radio, Init, Err
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

    LogBlob(LogType t = Init, uint32_t& sn = log_sn): type{t}, sn{sn++} {};

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

    template <typename WriteClass, uint8_t BUF_DIM>
    size_t write_bin(BufferedPrintPlus<WriteClass, BUF_DIM>& bp) {
        return bp.write(reinterpret_cast<const char*>(this), sizeof(LogBlob));
        // return 0;
    }

    static LogBlob blob_from_bin(uint32_t& sn, FsFile& file, const char *bin_name) {
        int size = sizeof(LogBlob);
        char* buf[size] = {0};
        int n = read_file(file, buf, size, bin_name);
        
        if (n == size) {
            LogBlob b(Init, sn);
            memcpy(&b, buf, size);
            return b;
        } 
        else if (n < size && n >= 0) Serial.println("end of file reached");
        else if (n < 0) Serial.println("file read error");
        return LogBlob(Err, sn);
    }
};

void blob_read_bin(const char *bin_name = "daq.bin", const char *csv_name = "daq_unpacked.csv") {
    uint32_t sn = 0;
    BufferedPrintPlus<FsFile, 255> bp(true, 256);
    FsFile f_bin, f_csv;

    if (!sd_open(PC4, PA6, PA7, PA5, f_bin, O_RDONLY, bin_name)) { Serial.println("Error opening bin log"); return; }
    if (!file_open(f_csv, csv_name, FILE_OVERWRITE)) { Serial.println("Error opening target csv"); return; }
    bp.begin(&f_csv);

    Serial.printf("bin >> csv, estimated output %d kB ...", f_bin.available() * 2 / 1024);
    while (f_bin.available()) {
        LogBlob b = LogBlob::blob_from_bin(sn, f_bin, bin_name);
        CSV_Line l;
        b.to_csv_line(l);
        l.write_row(bp);
    }
    
    f_bin.sync(); f_bin.close(); f_csv.close();
    SD.end();
    Serial.println("done");
}