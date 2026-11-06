#include "LightSensor.h"
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "AdcReader.h"

LightSensor::LightSensor(uint8_t i2cAddr)
    : meter(i2cAddr), addr(i2cAddr), ok(false), lux(0.0f), lastMs(0) {}

#ifdef SIMULATION
// ---------------------------------------------------------------------------
// WERSJA SYMULACYJNA: Wokwi nie ma BH1750, wiec swiatlo "mierzy" potencjometr.
// ---------------------------------------------------------------------------
bool LightSensor::begin() { //symulacja na podstawie potencjometru w wokwi
    adcInitPin(PIN_ADC_LIGHT_SIM);
    ok = true;
    lastMs = millis();
    Serial.println("SYMULACJA: swiatlo odczytywane z potencjometru (BH1750 nie istnieje w Wokwi)");
    return true;
}

void LightSensor::update() { //przeliczenie symulowanego odczytu swiatla z potencjometru
    const uint32_t now = millis();
    if (now - lastMs < READ_INTERVAL_MS) return;
    lastMs = now;
    lux = analogRead(PIN_ADC_LIGHT_SIM) * 65535.0f / 4095.0f;
}

#else
// ---------------------------------------------------------------------------
// WERSJA SPRZETOWA: prawdziwy BH1750 na magistrali I2C
// ---------------------------------------------------------------------------
bool LightSensor::begin() {
    // czujnik sam mierzy w tle, my tylko odczytujemy wynik
    ok = meter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addr, &Wire);
    if (!ok) Serial.println("BLAD: Nie wykryto czujnika swiatla BH1750 na magistrali I2C!");
    lastMs = millis();
    return ok;
}

void LightSensor::update() { // odczyt raz na sekunde
    const uint32_t now = millis();

    if (!ok) {   // czujnik niedostepny - probujemy co kilka sekund 
        if (now - lastMs < RETRY_INTERVAL_MS) return;
        lastMs = now;
        ok = meter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, addr, &Wire);
        if (ok) Serial.println("BH1750: czujnik swiatla ponownie dostepny");
        return;
    }

    if (now - lastMs < READ_INTERVAL_MS) return; 
    lastMs = now;

    const float v = meter.readLightLevel();
    if (v < 0.0f) {          // zwracany błąd przy ujemnej wartości przy bledzie odczytu
        ok = false;
        lux = 0.0f;
        Serial.println("BLAD: odczyt z BH1750 nieudany");
        return;
    }
    lux = v;
}
#endif
