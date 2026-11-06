#include "TurbiditySensor.h"
#include <Arduino.h>
#include "AdcReader.h"
#include "SensorMath.h"

TurbiditySensor::TurbiditySensor(int pin, int filterSamples, float divider, float vClear)
    : pin(pin), filter(filterSamples), divider(divider), vClear(vClear),
      lastMs(0), sensorVolts(0.0f), clarity(0.0f) {}

void TurbiditySensor::begin() { adcInitPin(pin); }

void TurbiditySensor::update() {
    const uint32_t now = millis();
    if (now - lastMs < SAMPLE_INTERVAL_MS) return;
    lastMs = now;

    const float adcMv = filter.addSample(adcReadMilliVolts(pin));   // filtr na napieciu z ADC
    sensorVolts = adcMv / 1000.0f * divider;                        // cofniecie dzielnika
    clarity = SensorMath::turbidityClarityPercent(sensorVolts, vClear);
}
