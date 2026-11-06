#pragma once
#include <stdint.h>
#include <BH1750.h>

// Czujnik natezenia swiatla BH1750, magistrala Wire musi byc wczesniej uruchomiona (Wire.begin).
class LightSensor {
public:
    explicit LightSensor(uint8_t i2cAddr);
    bool  begin();            // zwraca false, gdy czujnika nie ma na magistrali
    void  update();           // odczyt raz na sekunde

    float getLux() const { return lux; }
    bool  isOk()   const { return ok; }

private:
    static constexpr uint32_t READ_INTERVAL_MS   = 1000;
    static constexpr uint32_t RETRY_INTERVAL_MS  = 5000;   // ponowna inicjalizacja po utracie czujnika

    BH1750   meter;
    uint8_t  addr;
    bool     ok;
    float    lux;
    uint32_t lastMs;
};
