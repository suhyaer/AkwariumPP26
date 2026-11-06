#pragma once
#include <stdint.h>
#include "MovingAverage.h"

class TdsSensor {  // klasa do odczytu TDS (Total Dissolved Solids). Wzorcowanie w bringup.cpp.
public:
    TdsSensor(int pin, int filterSamples, float kValue);
    void begin();
    void update(float waterTempC);      // wywolywana w loop(); sama pilnuje odstepu miedzy probkami

    float getVoltageMv() const { return voltageMv; }   // napiecie po filtrze [mV]
    float getTdsPpm()    const { return tdsPpm; }

private:
    static constexpr uint32_t SAMPLE_INTERVAL_MS = 100;   // 10 probek/s
    int pin;
    MovingAverage filter;
    float kValue;
    uint32_t lastMs;
    float voltageMv;
    float tdsPpm;
};
