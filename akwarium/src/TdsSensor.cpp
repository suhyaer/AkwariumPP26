#include "TdsSensor.h"
#include <Arduino.h>
#include "AdcReader.h"
#include "SensorMath.h"

TdsSensor::TdsSensor(int pin, int filterSamples, float kValue) //inicjalizacja obiektu
    : pin(pin), filter(filterSamples), kValue(kValue),
      lastMs(0), voltageMv(0.0f), tdsPpm(0.0f) {}

void TdsSensor::begin() { adcInitPin(pin); }

void TdsSensor::update(float waterTempC) { //odczyt i przeliczenie TDS, wywolywane w loop() 
    const uint32_t now = millis();
    if (now - lastMs < SAMPLE_INTERVAL_MS) return;
    lastMs = now;

    // Filtrujemy NAPIECIE (przed przeliczeniem), bo wzor TDS jest nieliniowy
    voltageMv = filter.addSample(adcReadMilliVolts(pin));

    // Gdy brak waznego odczytu temperatury (-127 / bład), liczymy tak, jakby bylo 25 C
    const float t = (waterTempC > -100.0f) ? waterTempC : 25.0f;
    tdsPpm = SensorMath::tdsFromVoltage(voltageMv / 1000.0f, t, kValue);
}
