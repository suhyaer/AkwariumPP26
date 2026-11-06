#pragma once
#include <stdint.h>
#include "MovingAverage.h"

class TurbiditySensor {
public:
    // divider  - krotnosc dzielnika napiecia (2.0 dla 2x10k)
    // vClear   - napiecie czujnika w czystej wodzie [V]
    TurbiditySensor(int pin, int filterSamples, float divider, float vClear);
    void begin();
    void update();

    float getSensorVoltage()   const { return sensorVolts; }   // napiecie na wyjsciu czujnika [V]
    float getClarityPercent()  const { return clarity; }       // 100% = tak czysto jak przy kalibracji

private:
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 100;
    int pin;
    MovingAverage filter;
    float divider;
    float vClear;
    uint32_t lastMs;
    float sensorVolts;
    float clarity;
};
