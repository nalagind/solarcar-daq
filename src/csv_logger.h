#pragma once

#include <SdFat.h>
#include <type_traits>
#include "..\lib\SdFat\src\BufferedPrint.h"

enum CSV_Header {
    datestamp,
    timestamp,
    sn,
    log_type,

    can_ID,
    can_DLC,
    can_remote_request,
    
    can_node_name,

    can_raw_D0,
    can_raw_D1,
    can_raw_D2,
    can_raw_D3,
    can_raw_D4,
    can_raw_D5,
    can_raw_D6,
    can_raw_D7,

    gps_sat,
    gps_hdop,
    gps_lat,
    gps_lng,
    gps_loc_age,
    gps_alt_m,
    gps_spd_kmph,

    // daq_susp_FL_rpm,
    // daq_susp_FL_temp,
    
    // daq_susp_FR_rpm,
    // daq_susp_FR_temp,
    
    // daq_susp_RL_rpm,
    // daq_susp_RL_temp,
    
    // daq_susp_RR_rpm,
    // daq_susp_RR_temp,

    // comment,

    LAST,
};

const char* csv_header(CSV_Header header) {
    switch(header) {
        case datestamp: return "date";
        case timestamp: return "time";
        case sn: return "sn";
        case log_type: return "type";
        
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

        case gps_sat: return "gps_satellites";
        case gps_hdop: return "gps_hdop";
        case gps_lat: return "latitude";
        case gps_lng: return "longitude";
        case gps_loc_age: return "gps_fix_age";
        case gps_alt_m: return "gps_altitude_m";
        case gps_spd_kmph: return "gps_speed_kmph";

        // case daq_susp_FL_rpm: return "daq_susp_FL_rpm";
        // case daq_susp_FL_temp: return "daq_susp_FL_temp";

        // case daq_susp_FR_rpm: return "daq_susp_FR_rpm";
        // case daq_susp_FR_temp: return "daq_susp_FR_temp";

        // case daq_susp_RL_rpm: return "daq_susp_RL_rpm";
        // case daq_susp_RL_temp: return "daq_susp_RL_temp";

        // case daq_susp_RR_rpm: return "daq_susp_RR_rpm";
        // case daq_susp_RR_temp: return "daq_susp_RR_temp";

        // case comment: return "comment";

        case LAST: ;
        default: return "?";
    }
}

enum DataType {
    Int, UInt8_T, UInt32_T, UInt32_T_Hex, Float, Char_Ptr
};

union DataUnion {
    int i;
    uint8_t ui8;
    uint32_t ui32;
    float f;
    char* s;
};

enum BPBase { B_DEC, B_HEX };

template <typename T>
class has_sync {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().sync(), std::true_type());

    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = std::is_same<decltype(test<T>(0)), std::true_type>::value;
};

template <typename WriteClass, uint8_t BUF_DIM>
class BufferedPrintPlus: public BufferedPrint<WriteClass, BUF_DIM> {
private:
    WriteClass* m_wr;
    bool write_enabled = true;
    bool writeclass_sync_enabled = false;
    uint16_t writeclass_sync_cycle;
    uint16_t writeclass_write_count = 0;
    uint16_t writeclass_sync_count = 0;

public:
    bool sync() override {
        if (!write_enabled) return true;

        bool r = BufferedPrint<WriteClass, BUF_DIM>::sync();

        if (writeclass_sync_enabled) {
            if (r) writeclass_write_count++;
            if (writeclass_write_count >= writeclass_sync_cycle) {
                syncV<WriteClass>();
                writeclass_write_count = 0;
            }
        }

        return r;
    }

    template <typename U>
    void syncV() {
        if constexpr (has_sync<U>::value) {
          m_wr->sync();
          writeclass_sync_count++;
        }
    }

    size_t write(const void* src, size_t n) override {
        if (!write_enabled) return n;

        return BufferedPrint<WriteClass, BUF_DIM>::write(src, n);
    }

    template <typename Type>
    size_t printFieldHex(Type n, char term) {
        const uint8_t DIM = 13;
        char buf[DIM];
        char* str = buf + sizeof(buf);

        if (term) {
            *--str = term;
            if (term == '\n') {
                *--str = '\r';
            }
        }
        Type p = n < 0 ? -n : n;
        str = fmtHex(str, (uint32_t)p);
        if (n < 0) {
            *--str = '-';
        }
        return write(str, buf + sizeof(buf) - str);
    }

    void begin(WriteClass* wr) {
        m_wr = wr;
        BufferedPrint<WriteClass, BUF_DIM>::begin(wr);
    }

    void enable_write(bool e = true) { write_enabled = e; }

    void config_sync(bool sync_e, uint16_t sync_c) {
        writeclass_sync_enabled = sync_e;
        writeclass_sync_cycle = sync_c;
    }

    uint16_t get_wr_sync_count() { return writeclass_sync_count; }

    BufferedPrintPlus(bool sync_e = false, uint16_t sync_c = 1, bool write_e = true):
        BufferedPrint<WriteClass, BUF_DIM>(),
        m_wr(nullptr),
        writeclass_sync_enabled{sync_e},
        writeclass_sync_cycle{sync_c},
        write_enabled(write_e) 
    {}

    explicit BufferedPrintPlus(WriteClass* wr, bool sync_e = false, uint16_t sync_c = 1, bool write_e = true):
        BufferedPrint<WriteClass, BUF_DIM>(wr),
        m_wr(wr),
        writeclass_sync_enabled{sync_e},
        writeclass_sync_cycle{sync_c},
        write_enabled(write_e)
    {}

    friend class CSV_Line;
};

struct CSV_Line {
private:
    int front = 0;
    uint8_t headers[CSV_Header::LAST] = {0};
    DataType types[CSV_Header::LAST];
    DataUnion values[CSV_Header::LAST];

    const CSV_Header no_fltr[1] = {CSV_Header::LAST};

public:
    void append(CSV_Header header, int value) {
        headers[header] = 1;
        types[header] = DataType::Int;
        values[header].i = value;
        front++;
    }

    void append(CSV_Header header, uint8_t value) {
        headers[header] = 1;
        types[header] = DataType::UInt8_T;
        values[header].ui8 = value;
        front++;
    }

    void append(CSV_Header header, uint32_t value, BPBase b = B_DEC) {
        headers[header] = 1;
        if (b == B_DEC) types[header] = DataType::UInt32_T;
        else if (b == B_HEX) types[header] = DataType::UInt32_T_Hex;
        values[header].ui32 = value;
        front++;
    }

    void append(CSV_Header header, float value) {
        headers[header] = 1;
        types[header] = DataType::Float;
        values[header].f = value;
        front++;
    }

    template <size_t N>
    void append(CSV_Header header, const char (&value)[N]) {
        headers[header] = 1;
        types[header] = DataType::Char_Ptr;
        char* s = new char[N];
        strcpy(s, value);
        values[header].s = s;
        front++;
    }

    void append(CSV_Header header, const char* value) {
        headers[header] = 1;
        types[header] = DataType::Char_Ptr;
        char* s = new char[strlen(value) + 1];
        strcpy(s, value);
        values[header].s = s;
        front++;
    }

    template <typename WriteClass, uint8_t BUF_DIM>
    void write_row(BufferedPrintPlus<WriteClass, BUF_DIM>& bp, bool with_header = false, bool aligned = true) {
        write_row(bp, no_fltr, with_header, aligned);
    }

    template <typename WriteClass, uint8_t BUF_DIM, size_t N>
    void write_row(BufferedPrintPlus<WriteClass, BUF_DIM>& bp, const CSV_Header (&filters)[N], bool with_header = false, bool aligned = true, char term = ',') {
        if (bp.write_enabled == false) return;
        
        bool filterred = (N != 1 && filters[0] != LAST);
        int max = filterred ? N : LAST;
        int _front = front;
        
        for (int j = 0; j < max; j++) {
            if (_front <= 0) break;
            int h = filterred ? filters[j] : j;

            if (headers[h] == 1) {
                if (with_header) bp.printField(csv_header((CSV_Header)h), ' ');
                switch (types[h]) {
                    case Int: { bp.printField(values[h].i, term); break; }
                    case UInt8_T: { bp.printField(values[h].ui8, term); break; }
                    case UInt32_T: { bp.printField(values[h].ui32, term); break; }
                    case UInt32_T_Hex: { bp.print("0x"); bp.printFieldHex(values[h].ui32, term); break; }
                    case Float: { bp.printField(values[h].f, term); break; }
                    case Char_Ptr: { bp.printField(const_cast<const char*>(values[h].s), term); break; }
                    default: { bp.printField("NAN", term); };
                }
                if (with_header) bp.print(' ');
                _front--;
            } else if (!filterred && aligned) bp.print(term);
        }
        bp.println();
    }

    ~CSV_Line() {
        for (int i = 0; i < CSV_Header::LAST; i++) {
            if (headers[i] == 1) {
                switch (types[i]) {
                    case Char_Ptr: delete[] values[i].s;
                    default: ;
                }
            }
        }   
    }

    template <size_t N>
    static void make_filter(CSV_Header (&filters)[N]) {
        std::sort(filters, filters + N);
    }
};
