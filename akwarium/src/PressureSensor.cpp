#include "PressureSensor.h"
#include <Arduino.h>
#include "config.h"
#include "SensorMath.h"
#include "AdcReader.h"

#ifndef SIMULATION
static portMUX_TYPE hxMux = portMUX_INITIALIZER_UNLOCKED; //chroni odczyt przed przerwaniem
#endif

PressureSensor::PressureSensor(int dataPin, int clockPin, int filterSamples) //inicjalizacja obiektu
    : dataPin(dataPin), clockPin(clockPin), filter(filterSamples),
      lastMs(0), lastSampleMs(0), everSampled(false), rawFiltered(0.0f), kpa(0.0f) {}

void PressureSensor::begin() {
#ifdef SIMULATION // symulacja na podstawie potencjometru w wokwi
    adcInitPin(dataPin);
#else
    pinMode(dataPin, INPUT);
    pinMode(clockPin, OUTPUT);
    digitalWrite(clockPin, LOW);   // SCK w stanie niskim = HX710B pracuje (HIGH > 60 us = uspienie)
#endif
}

#ifndef SIMULATION
// Odczyt jednej probki z HX710B. DOUT=LOW oznacza "dane gotowe".
// 24 impulsy SCK = 24 bity (MSB pierwszy), potem HX_EXTRA_PULSES wybiera tryb nastepnego pomiaru.
bool PressureSensor::readHx(int32_t& out) {
    if (digitalRead(dataPin) != LOW) return false;   // przetwornik mierzy

    uint32_t v = 0;
    portENTER_CRITICAL(&hxMux);   // przerwanie (np. Wi-Fi) w srodku odczytu zaklocilo
    for (int i = 0; i < 24; i++) {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(1);
        v = (v << 1) | (uint32_t)digitalRead(dataPin);
        digitalWrite(clockPin, LOW);
        delayMicroseconds(1);
    }
    for (int i = 0; i < HX_EXTRA_PULSES; i++) {
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(clockPin, LOW);
        delayMicroseconds(1);
    }
    portEXIT_CRITICAL(&hxMux);

    out = SensorMath::signExtend24(v);
    return true;
}
#endif

void PressureSensor::update() {  //odczyt i przeliczenie
    const uint32_t now = millis();
    if (now - lastMs < SAMPLE_INTERVAL_MS) return;
    lastMs = now;

    float raw;
#ifdef SIMULATION // symulacja na podstawie potencjometru w wokwi
    raw = (float)analogRead(dataPin);          // 0..4095
#else
    int32_t r;
    if (!readHx(r)) return;                    // brak swiezej probki - nic nie robimy
    raw = (float)r;
#endif
                                            //warunki zakonczenia
    rawFiltered  = filter.addSample(raw); 
    kpa          = SensorMath::pressureKpa(rawFiltered, PRESSURE_RAW_ZERO, PRESSURE_RAW_PER_KPA);
    lastSampleMs = now;
    everSampled  = true;
}

bool PressureSensor::isOk() const {
    return everSampled && (millis() - lastSampleMs < STALE_AFTER_MS);
}
