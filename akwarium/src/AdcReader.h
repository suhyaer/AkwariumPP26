#pragma once
#include <Arduino.h>

// odczyt sygnalow analogowych zakres do ok. 3.1 V (tlumienie 11 dB), 12 bitow
inline void adcInitPin(int pin) {
    pinMode(pin, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(pin, ADC_11db);
}

// Odczyt napiecia [mV] z usrednianiem 'samples' pomiarow za pomoca petli (redukuje szum szybki).
inline float adcReadMilliVolts(int pin, int samples = 16) {
    float acc = 0.0f;
    for (int i = 0; i < samples; i++) {
#ifdef SIMULATION  // dla symulacji
        acc += analogRead(pin) * 3300.0f / 4095.0f;
#else
        acc += (float)analogReadMilliVolts(pin);
#endif
    }
    return acc / samples;
}
