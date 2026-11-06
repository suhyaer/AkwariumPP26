#pragma once
// Czyste funkcje przeliczeniowe (bez zaleznosci od sprzetu) - dzieki temu daja sie testowac.
#include <stdint.h>

namespace SensorMath {

// TDS [ppm] z napiecia czujnika [V] z kompensacja temperatury (wzor producenta DFRobot SEN0244).
// Przewodnosc wody rosnie ok. 2% na 1 C, wiec napiecie sprowadzamy do 25 C.
inline float tdsFromVoltage(float volts, float tempC, float kValue = 1.0f) {
    const float coef = 1.0f + 0.02f * (tempC - 25.0f);
    const float v = volts / coef;
    const float ppm = (133.42f * v * v * v - 255.86f * v * v + 857.39f * v) * 0.5f * kValue;
    return ppm < 0.0f ? 0.0f : ppm;
}

// "Przejrzystosc" [%] wzgledem napiecia zmierzonego w czystej wodzie (100% = tak czysto jak przy kalibracji).
inline float turbidityClarityPercent(float sensorVolts, float clearVolts) {
    if (clearVolts <= 0.0f) return 0.0f;
    float p = sensorVolts / clearVolts * 100.0f;
    if (p < 0.0f) p = 0.0f;
    if (p > 100.0f) p = 100.0f;
    return p;
}

// 24-bitowy wynik HX710B/HX711 (kod uzupelnien do dwoch) -> int32
inline int32_t signExtend24(uint32_t raw) {
    raw &= 0xFFFFFFu;
    if (raw & 0x800000u) raw |= 0xFF000000u;
    return (int32_t)raw;
}

inline float pressureKpa(float raw, float zero, float perKpa) {
    return (perKpa == 0.0f) ? 0.0f : (raw - zero) / perKpa;
}


//  Kalibracja 
// Wspolczynnik k dla TDS: ile razy trzeba przeskalowac wynik liczony z k=1, zeby zgadzal sie z wzorcem.
inline float tdsKFromReference(float referencePpm, float measuredPpmAtK1) {
    return (measuredPpmAtK1 <= 0.0f) ? 0.0f : referencePpm / measuredPpmAtK1;
}

// Czulosc czujnika cisnienia: ile jednostek "raw" przypada na 1 kPa .
inline float pressureRawPerKpa(float rawAtPressure, float rawZero, float knownKpa) {
    return (knownKpa == 0.0f) ? 0.0f : (rawAtPressure - rawZero) / knownKpa;
}

// Cisnienie slupa wody o wysokosci h [cm] w kPa (ok. 0.0981 kPa na 1 cm).
inline float waterColumnKpa(float heightCm) { return heightCm * 0.0980665f; }

}  // namespace SensorMath
