#pragma once
#include <stdint.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

class TemperatureManager {
public:
    // oneWirePin          - pin magistrali 1-Wire (DS18B20)
    // relayPin            - pin sterujacy przekaznikiem grzalki
    // defaultTarget       - domyslna temperatura zadana [°C] (gdy NVS jest puste)
    // hysteresisHalfBand  - POLOWA pasma histerezy [°C]: grzalka wlacza sie przy
    //                       target - h, a wylacza przy target + h
    // relayActiveHigh     - true: HIGH wlacza grzalke, false: LOW wlacza grzalke
    //                       (wiele modulow przekaznikowych jest aktywnych stanem LOW)
    TemperatureManager(int oneWirePin, int relayPin, float defaultTarget,
                       float hysteresisHalfBand, bool relayActiveHigh = true);

    // Inicjalizacja sprzetu i wczytanie danych z NVS
    void begin();

    // wywolanie cyklicznie w loop(). Nie blokuje programu.
    void update();

    // Zmiana temperatury zadanej z zapisem do NVS (tylko gdy wartosc sie zmienila).
    // Zwraca false, gdy wartosc jest nieprawidlowa (NaN / poza zakresem).
    bool setTargetTemperature(float newTarget);

    float getCurrentTemp() const;   // blad gdy brak waznego odczytu
    float getTargetTemp() const;
    bool  getHeaterState() const;
    bool  isSensorOk() const;       // czy ostatni odczyt byl poprawny

    // Limity bezpieczenstwa
    static constexpr float MIN_TARGET_TEMP     = 15.0f;  // najnizsza dozwolona temp. zadana
    static constexpr float MAX_TARGET_TEMP     = 32.0f;  // najwyzsza dozwolona temp. zadana
    static constexpr float ABSOLUTE_MAX_TEMP   = 35.0f;  // powyzej tej temp. grzalka zawsze OFF

private:
    static constexpr uint32_t READ_INTERVAL_MS      = 1000;   // co ile zlecac nowy pomiar
    static constexpr uint32_t ERROR_LOG_INTERVAL_MS = 10000;  // ograniczenie spamu w logu

    OneWire           oneWire;
    DallasTemperature sensor;
    Preferences       preferences;

    int   relayPin;  
    bool  relayActiveHigh;
    float defaultTarget;
    float targetTemp;
    float currentTemp;
    float hysteresis;
    bool  heaterState;
    bool  sensorOk;

    bool     conversionPending;
    uint32_t lastRequestMs;
    uint32_t lastErrorLogMs;
    uint32_t conversionTimeMs;

    void writeRelay(bool on);
    void turnHeaterOn();
    void turnHeaterOff();
    void handleSensorError(const char* reason);
    static bool isTargetValid(float t);
};
