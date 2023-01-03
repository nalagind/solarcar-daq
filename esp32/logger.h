#pragma once

class EventLogger {
    protected:
    struct tm tmstruct;
    struct timeval tv;
    int64_t time_ms;
    bool realtime = false;
    const char *dataSourceTaskName;
    unsigned long sn;

    public:
    EventLogger(const char *name, unsigned long sn) { //inherited classes: use macro for name
        dataSourceTaskName = name;
        this->sn = sn;
        setTime_ms();
    }

    int64_t setTime_ms() {
        if (getLocalTime(&tmstruct, 10)) {
            realtime = true;
            gettimeofday(&tv, NULL);
            time_ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
        } else {
            time_ms = millis();
        }

        return time_ms;
    }

    int64_t getTime_ms() { return realtime ? time_ms : 0; }

    virtual void generateLine(char *line, const char *delimiter = ",") {}
};
