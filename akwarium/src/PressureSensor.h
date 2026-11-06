#pragma once
#include <stdint.h>
#include "MovingAverage.h"

// Czujnik cisnienia napowietrzania.
//  - SPRZET:      MPS20N0040D + modul HX710B (24-bitowy przetwornik, interfejs 2-przewodowy DOUT/SCK)
//  - SYMULACJA:   (-DSIMULATION) potencjometr na wejsciu ADC - HX710B nie istnieje w Wokwi
class PressureSensor {
public:
    PressureSensor(int dataPin, int clockPin, int filterSamples);
    void begin();
    void update();                       // nieblokujace

    float getRaw() const { return rawFiltered; }
    float getKpa() const { return kpa; }
    bool  isOk()   const;                // czy w ostatniej sekundzie przyszla swieza probka

private:
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 100;
    static constexpr uint32_t STALE_AFTER_MS     = 1000;

    int dataPin;
    int clockPin;
    MovingAverage filter;
    uint32_t lastMs;
    uint32_t lastSampleMs;
    bool     everSampled;
    float    rawFiltered;
    float    kpa;

#ifndef SIMULATION
    bool readHx(int32_t& out);
#endif
};
