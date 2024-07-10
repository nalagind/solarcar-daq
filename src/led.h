#pragma once
#include <Arduino.h>

#if defined(TIMECONSUMER)
#define LED_R PB7
#define LED_G PB6
#define LED_B PB5
#elif defined(UNIFIER)
#define LED1_R PC3
#define LED1_G PB2
#define LED1_B PB1

#define LED2_R PA15
#define LED2_G PC8
#define LED2_B PC7

#define LED3_R PC15
#define LED3_G PC14
#define LED3_B PC13

#define LED_R LED1_R
#define LED_G LED1_G
#define LED_B LED1_B
#endif

enum led {
    R, G, B, C, Y, W, LED_LAST
};

struct LEDColors {
private:
    // std::vector<bool> states;
    // std::vector<uint32_t> leds;
    bool states[LED_LAST] = {false};
    uint32_t leds[3];

public:
    LEDColors(uint32_t r, uint32_t g, uint32_t b) {
        leds[0] = r; leds[1] = g; leds[2] = b;
        for (auto l: leds) {
            pinMode(l, OUTPUT);
            digitalWrite(l, HIGH);
        }
    }

    void toggle(led l) {
        for (auto i: leds) {
            pinMode(i, OUTPUT);
            digitalWrite(i, HIGH);
        }
        for (int s = 0; s < LED_LAST; s++) {
            if (s != l) states[s] = false;
            else states[s] = !(states[s]);
        }
        switch (l) {
            case R:
                digitalWrite(leds[l], !(states[l]));
                return;
            case G:
                digitalWrite(leds[l], !(states[l]));
                return;
            case B:
                digitalWrite(leds[l], !(states[l]));
                return;
            case C:
                if (states[l]) {
                    analogWrite(leds[B], 220);
                    analogWrite(leds[G], 192);
                }
                return;
            case Y:
                if (states[l]) {
                    analogWrite(leds[R], 25);
                    analogWrite(leds[G], 192);
                }
                return;
            case W:
                if (states[l]) {
                    analogWrite(leds[R], 125);
                    analogWrite(leds[B], 220);
                    analogWrite(leds[G], 192);
                }
                return;
            default: ;
        };
    }

    void turn_on(led l) {
        for (auto i: leds) {
            pinMode(i, OUTPUT);
            digitalWrite(i, HIGH);
        }
        for (auto s: states) s = false;
        states[l] = true;
        switch (l) {
            case R:
                digitalWrite(leds[l], LOW);
                return;
            case G:
                digitalWrite(leds[l], LOW);
                return;
            case B:
                digitalWrite(leds[l], LOW);
                return;
            case C:
                analogWrite(leds[B], 220);
                analogWrite(leds[G], 192);
                return;
            case Y:
                analogWrite(leds[R], 25);
                analogWrite(leds[G], 192);
                return;
            case W:
                analogWrite(leds[R], 125);
                analogWrite(leds[B], 220);
                analogWrite(leds[G], 192);
                return;
            default: ;
        }
    }

    void turn_off() {
        for (auto i: leds) {
            pinMode(i, OUTPUT);
            digitalWrite(i, HIGH);
        }
        for (auto s: states) s = false;
    }

    bool get_state(led l) { return states[l]; }
};

LEDColors LED(LED_R, LED_B, LED_G);
#if defined(UNIFIER)
LEDColors LED2(LED2_R, LED2_B, LED2_G);
LEDColors LED3(LED3_R, LED3_B, LED3_G);
#endif