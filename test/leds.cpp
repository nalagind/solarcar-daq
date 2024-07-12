#include <Arduino.h>
#include "../src/led.h"

void setup() {
    // pinMode(LED_B, OUTPUT); digitalWrite(LED_B, HIGH);
    // pinMode(LED_G, OUTPUT); digitalWrite(LED_G, HIGH);
    // pinMode(LED_R, OUTPUT); digitalWrite(LED_R, HIGH);


    //yellow
    // analogWrite(LED_R, 25);
    // analogWrite(LED_G, 192);

    //white
    // analogWrite(LED_R, 125);
    // analogWrite(LED_B, 220);
    // analogWrite(LED_G, 192);

    //cyan
    // analogWrite(LED_B, 220);
    // analogWrite(LED_G, 192);


    // for (uint32_t i = 0; i < 255; i = (i + 1) * 31) {
    //     analogWrite(LED_G, i);
    //     delay(1000);
    // }
    // analogWrite(LED_B, 220);
    // delay(1000);
    // pinMode(LED_B, OUTPUT);
    // digitalWrite(LED_B, HIGH);
}

void loop() {
    // LED.toggle(R);
    // delay(1000);

    LED.toggle(R);
    delay(1000);
    LED.toggle(G);
    delay(1000);
    LED.toggle(B);
    delay(1000);
    LED.turn_off();
    delay(1000);
    // LED.turn_off();
    // delay(1000);
    LED.turn_on(C);
    delay(1000);
    LED.toggle(Y);
    delay(1000);
    LED.turn_on(W);
    delay(1000);
    LED.turn_on(W);
    delay(1000);

    // LED.turn_off(W);
    // delay(1000);
    // LED.toggle(B);
    // delay(1000);
    // LED.toggle(C);
    // delay(1000);
    // LED.toggle(Y);
    // delay(1000);
    // LED.toggle(W);
    // delay(1000);
}