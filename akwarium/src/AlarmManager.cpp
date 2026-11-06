#include "AlarmManager.h"
#include "config.h"

void AlarmManager::update(const SystemStatus& s, uint32_t now) {
    bool c[ALARM_COUNT];
    // Alarm wartosci liczymy tylko, gdy odpowiedni czujnik daje wazny odczyt inaczej
    // awaria czujnika wywolalaby falszywy alarm "cisnienie 0" lub "temperatura -127".
    c[ALARM_TEMP_SENSOR]     = !s.tempOk;
    c[ALARM_TEMP_HIGH]       = s.tempOk && s.tempC > s.targetC + TEMP_ALARM_HIGH_DELTA;
    c[ALARM_TEMP_LOW]        = s.tempOk && s.tempC < s.targetC - TEMP_ALARM_LOW_DELTA;
    c[ALARM_AERATION_LOW]    = s.pressureOk && s.pressureKpa < AERATION_MIN_KPA;
    c[ALARM_AERATION_HIGH]   = s.pressureOk && s.pressureKpa > AERATION_MAX_KPA;
    c[ALARM_TURBID]          = s.clarityPct < TURBIDITY_ALARM_CLARITY_PCT;
    c[ALARM_TDS_HIGH]        = s.tdsPpm > TDS_ALARM_PPM;
    c[ALARM_PRESSURE_SENSOR] = !s.pressureOk;
    c[ALARM_LIGHT_SENSOR]    = !s.lightOk;

    const uint32_t onDelay[ALARM_COUNT] = { // przypisuje czas opóźnienia dla każdego alarmu w milisekundach
        ALARM_TEMP_SENSOR_DELAY_MS,    // TEMP_SENSOR
        ALARM_TEMP_HIGH_DELAY_MS,      // TEMP_HIGH
        ALARM_TEMP_LOW_DELAY_MS,       // TEMP_LOW
        ALARM_AERATION_DELAY_MS,       // AERATION_LOW
        ALARM_AERATION_DELAY_MS,       // AERATION_HIGH
        ALARM_TURBID_DELAY_MS,         // TURBID
        ALARM_TDS_DELAY_MS,            // TDS_HIGH
        ALARM_PRESSURE_SENSOR_DELAY_MS,// PRESSURE_SENSOR
        ALARM_LIGHT_SENSOR_DELAY_MS    // LIGHT_SENSOR
    };

    uint16_t m = 0;
    for (uint8_t i = 0; i < ALARM_COUNT; i++) {
        if (deb[i].update(c[i], now, onDelay[i], ALARM_CLEAR_DELAY_MS)) m |= (uint16_t)(1u << i);
    }
    mask = m;
}

uint8_t AlarmManager::count() const { // liczba aktywnych alarmow
    uint8_t n = 0;
    for (uint8_t i = 0; i < ALARM_COUNT; i++) if ((mask >> i) & 1u) n++;
    return n;
}

const char* AlarmManager::name(AlarmId id) { // krotki opis ASCII na OLED (<= 21 znakow)
    switch (id) {
        case ALARM_TEMP_SENSOR:     return "Brak czujnika temp.";
        case ALARM_TEMP_HIGH:       return "Temp. za wysoka";
        case ALARM_TEMP_LOW:        return "Temp. za niska";
        case ALARM_AERATION_LOW:    return "Brak napowietrzania";
        case ALARM_AERATION_HIGH:   return "Zator napowietrzania";
        case ALARM_TURBID:          return "Woda metna";
        case ALARM_TDS_HIGH:        return "TDS za wysokie";
        case ALARM_PRESSURE_SENSOR: return "Brak czujnika cisn.";
        case ALARM_LIGHT_SENSOR:    return "Brak czujnika swiatla";
        default:                    return "?";
    }
}

const char* AlarmManager::code(AlarmId id) { // identyfikator w JSON
    switch (id) {
        case ALARM_TEMP_SENSOR:     return "temp_sensor";
        case ALARM_TEMP_HIGH:       return "temp_high";
        case ALARM_TEMP_LOW:        return "temp_low";
        case ALARM_AERATION_LOW:    return "aeration_low";
        case ALARM_AERATION_HIGH:   return "aeration_high";
        case ALARM_TURBID:          return "turbid";
        case ALARM_TDS_HIGH:        return "tds_high";
        case ALARM_PRESSURE_SENSOR: return "pressure_sensor";
        case ALARM_LIGHT_SENSOR:    return "light_sensor";
        default:                    return "unknown";
    }
}
