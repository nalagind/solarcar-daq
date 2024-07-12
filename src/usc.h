#pragma once
#include <Arduino.h>
#include "led.h"

enum USC_Pin {
    UART_TX, UART_RX,
    SPI_MOSI, SPI_MISO, SPI_CS, SPI_SCK,
    I2C_SDA, I2C_SCL,
    PWM,
    ANALOG_IN,
    ID
};

enum USC_Periph {
    UART, SPI, I2C
};

class USC {
private:
    std::unordered_map<USC_Pin, uint32_t> usc_map;
    bool status;
    LEDColors led;

public:
    USC(const std::unordered_map<USC_Pin, uint32_t>& m,
        LEDColors l): usc_map(m), led(l) {}

    uint32_t p(USC_Pin p) const {
        auto it = usc_map.find(p);
        if (it != usc_map.end()) return it->second;
        else return NC;
    }

    void update(bool s) { status = s; }
    bool get_stat() { return status; }
};

std::unordered_map<USC_Pin, uint32_t> uscL_map = {
    {UART_TX, PA2}, {UART_RX, PA3},
    {SPI_MOSI, PA7}, {SPI_MISO, PA6}, {SPI_CS, PC5}, {SPI_SCK, PA5},
    {I2C_SDA, PB9}, {I2C_SCL, PB10},
    {PWM, PB0}, {ANALOG_IN, PA4}, {ID, PC4}
};

std::unordered_map<USC_Pin, uint32_t> uscC_map = {
    {UART_TX, PA9}, {UART_RX, PA10},
    {SPI_MOSI, PB15}, {SPI_MISO, PB14}, {SPI_CS, PB12}, {SPI_SCK, PB13},
    {I2C_SDA, PC9}, {I2C_SCL, PA8},
    {PWM, PC6}, {ANALOG_IN, PA1}, {ID, PA0}
};

std::unordered_map<USC_Pin, uint32_t> uscR_map = {
    {UART_TX, PC10}, {UART_RX, PC11},
    {SPI_MOSI, PC12}, {SPI_MISO, PB4}, {SPI_CS, PD2}, {SPI_SCK, PB3},
    {I2C_SDA, PB7}, {I2C_SCL, PB8},
    {PWM, PB6}, {ANALOG_IN, PC0}, {ID, PC1}
};

USC uscL(uscL_map, LEDL);
USC uscC(uscC_map, LEDC);
USC uscR(uscR_map, LEDR);
USC uscs[] = {uscL, uscC, uscR};
