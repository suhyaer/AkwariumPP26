#pragma once
#include <stdint.h>

// Stan calego urzadzenia, wypelniana w main.cpp i przekazywana do OLED oraz serwera WWW.
struct SystemStatus {
    float tempC;        bool  tempOk;
    float targetC;      bool  heaterOn;
    float tdsPpm;       float tdsMv;
    float clarityPct;   float turbidityV;
    float lux;          bool  lightOk;
    float pressureKpa;  bool  pressureOk;
    bool  wifiConnected;
    int   wifiRssi;
    char  ip[16];           // adres IP jako tekst, np. "192.168.1.57" (pusty, gdy brak sieci)
    uint32_t uptimeS;
    uint16_t alarmMask;     // bit i = aktywny alarm AlarmId(i)
};
