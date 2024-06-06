#pragma once

#include <STM32RTC.h>
#include <SdFat.h>
#include "BufferedPrint.h"

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

    // comment,

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

        // case comment: return "comment";

        case LAST: return "";
    }
}

enum DataType {
    Int, UInt8_T, UInt32_T, Float, Char_Ptr
};

union DataUnion {
    int i;
    uint8_t ui8;
    uint32_t ui32;
    float f;
    char* s;
};

enum LogType {
    CAN, DAQ, Err
};

extern File file;

template <typename WriteClass, uint8_t BUF_DIM>
class DAQBufferedPrint: public BufferedPrint<WriteClass, BUF_DIM> {
private:
    bool write_enabled;
    uint32_t writeclass_write_count;

public:
    bool sync() {
        if (!write_enabled) {
            return true;
        }
        bool r = BufferedPrint<WriteClass, BUF_DIM>::sync();
        if (r) writeclass_write_count++;
        return r;
    }

    size_t write(const void* src, size_t n) {
        if (!write_enabled) {
            return n;
        }
        return BufferedPrint<WriteClass, BUF_DIM>::write(src, n);
    }

    void enable_write(bool e) { write_enabled = e; }

    explicit DAQBufferedPrint(WriteClass* wr, bool write_e = true):
        BufferedPrint<WriteClass, BUF_DIM>(wr), write_enabled(write_e), writeclass_write_count(0) {}

    uint32_t get_writeclass_write_count() { return writeclass_write_count; }
};

struct CSV_Row {
private:
    int front = 0;
    CSV_Header headers[10];
    DataType types[10];
    DataUnion values[10];
    bool organized = false;

    const CSV_Header no_fltr[1] = {CSV_Header::LAST};

public:
    void append(CSV_Header header, int value) {
        headers[front] = header;
        types[front] = DataType::Int;
        values[front].i = value;
        front++;
        organized = false;
    }

    void append(CSV_Header header, uint8_t value) {
        headers[front] = header;
        types[front] = DataType::UInt8_T;
        values[front].ui8 = value;
        front++;
        organized = false;
    }

    void append(CSV_Header header, uint32_t value) {
        headers[front] = header;
        types[front] = DataType::UInt32_T;
        values[front].ui32 = value;
        front++;
        organized = false;
    }

    void append(CSV_Header header, float value) {
        headers[front] = header;
        types[front] = DataType::Float;
        values[front].f = value;
        front++;
        organized = false;
    }

    template <size_t N>
    void append(CSV_Header header, const char (&value)[N]) {
        headers[front] = header;
        types[front] = DataType::Char_Ptr;
        char* s = new char[N];
        strcpy(s, value);
        values[front].s = s;
        front++;
        organized = false;
    }

    CSV_Row(uint32_t sn, LogType type) {
        append(CSV_Header::sn, sn);

        if (sn == 0) {
            char timestamp[18];
            snprintf(timestamp, sizeof(timestamp), "%02d/%02d/%02d %02d:%02d:%02d", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
            append(CSV_Header::timestamp, timestamp);
        } else {
            char timestamp[9];
            snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
            append(CSV_Header::timestamp, timestamp);
        }

        append(CSV_Header::log_type, type);
    }

    void organize() { organized = true; }

    template <typename WriteClass, uint8_t BUF_DIM>
    void write_row(DAQBufferedPrint<WriteClass, BUF_DIM>& bp) {
        write_row(bp, no_fltr);
    }

    template <typename WriteClass, uint8_t BUF_DIM, size_t N>
    void write_row(DAQBufferedPrint<WriteClass, BUF_DIM>& bp, CSV_Header (&filters)[N], char term = ',', bool with_header = false, bool aligned = true) {
        if (!organized) organize();

        int last_col = 0;
        size_t filters_count = (N == 1 && filters[0] == CSV_Header::LAST) ? 0 : N;
        if (filters_count) {
            for (int i = 0; i < front; i++) {
                for (int j = 0; i < filters_count; j++) {
                    if (headers[i] == filters[j]) {
                        if (with_header) {
                            bp.printField(csv_header(headers[i]), ' ');
                        }

                        switch (types[i]) {
                            case Int: { bp.printField(values[i].i, term); }
                            case UInt8_T: { bp.printField(values[i].ui8, term); }
                            case UInt32_T: { bp.printField(values[i].ui32, term); }
                            case Float: { bp.printField(values[i].f, term); }
                            case Char_Ptr: { bp.printField(values[i].s, term); }
                            default: { bp.printField("NAN", term); };
                        }
                    }
                }
            }
        } else {
            for (int i = 0; i < front; i++) {
                if (aligned) {
                    for (int j = last_col; j < headers[i]; j++) {
                        bp.print(term);
                    }
                }
                last_col = headers[i];

                if (with_header) {
                    bp.printField(csv_header(headers[i]), ' ');
                }

                switch (types[i]) {
                    case Int: { bp.printField(values[i].i, term); }
                    case UInt8_T: { bp.printField(values[i].ui8, term); }
                    case UInt32_T: { bp.printField(values[i].ui32, term); }
                    case Float: { bp.printField(values[i].f, term); }
                    case Char_Ptr: { bp.printField(values[i].s, term); }
                    default: { bp.printField("NAN", term); };
                }
            }
        }
        bp.println();
    }

    ~CSV_Row() {
        for (int i = 0; i < front; i++) {
            switch (types[i]) {
                case Char_Ptr: delete[] values[i].s;
                default: ;
            }
        }   
    }
};
