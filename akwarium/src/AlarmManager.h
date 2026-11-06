#pragma once
#include <stdint.h>
#include "SystemStatus.h"
#include "Debounce.h"

enum AlarmId : uint8_t {
    ALARM_TEMP_SENSOR = 0,   // brak odczytu temperatury (grzalka juz wylaczona przez TemperatureManager)
    ALARM_TEMP_HIGH,         // za cieplo wzgledem zadanej
    ALARM_TEMP_LOW,         // za zimno wzgledem zadanej, dluzej niz dopuszczalne
    ALARM_AERATION_LOW,      // za niskie cisnienie: pompka nie pracuje / wezyk odlaczony
    ALARM_AERATION_HIGH,     // za wysokie cisnienie: zator
    ALARM_TURBID,            // mętna woda
    ALARM_TDS_HIGH,          // za wysokie TDS
    ALARM_PRESSURE_SENSOR,   // brak odczytu czujnika cisnienia
    ALARM_LIGHT_SENSOR,      // brak odczytu czujnika swiatla
    ALARM_COUNT
};

// Ocena alarmow na podstawie migawki stanu. Czysta logika (bez sprzetu) - testowana samotestem.
// Alarmy NIE steruja grzalka: zabezpieczenia sprzetowe zostaja w TemperatureManager.
class AlarmManager {
public:
    AlarmManager() : mask(0) {}
    void update(const SystemStatus& s, uint32_t nowMs);

    bool     isActive(AlarmId id) const { return (mask >> id) & 1u; }
    uint16_t getMask() const { return mask; }
    uint8_t  count() const;

    static const char* name(AlarmId id);   // krotki opis ASCII na OLED (<= 21 znakow)
    static const char* code(AlarmId id);   // identyfikator w JSON

private:
    Debounce deb[ALARM_COUNT];
    uint16_t mask;
};
