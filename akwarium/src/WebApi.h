#pragma once
// Czesc logiki serwera WWW bez zaleznosci od sieci mozna ja przetestowac samotestem.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SystemStatus.h"
#include "AlarmManager.h"

namespace WebApi {

// Akceptuje kropke i przecinek ("26,5"), spacje na brzegach.
// Odrzuca nullptr, pusty tekst, smieci ("26.5x", "abc"), NaN i inf.
inline bool parseTargetParam(const char* s, float& out) {
    if (!s) return false;
    char buf[24];
    size_t i = 0;
    for (; *s && i < sizeof(buf) - 1; ++s, ++i) buf[i] = (*s == ',') ? '.' : *s;
    if (*s) return false;                       // za dlugi tekst
    buf[i] = '\0';
    if (i == 0) return false;

    char* end = nullptr;
    const float v = strtof(buf, &end);
    if (end == buf) return false;               // brak cyfr
    while (*end == ' ' || *end == '\t') ++end;
    if (*end != '\0') return false;             // smieci za liczba
    if (!isfinite(v)) return false;             // "nan", "inf"
    out = v;
    return true;
}

// Buduje JSON ze stanem urzadzenia. Wartosci bez waznego odczytu -> null.
// Zwraca liczbe znakow wynik >= size oznacza, ze bufor byl za maly.
inline int buildStatusJson(const SystemStatus& s, char* buf, size_t size) {
    char t[16], lx[16], pr[16];
    if (s.tempOk)     snprintf(t,  sizeof(t),  "%.2f", s.tempC);       else snprintf(t,  sizeof(t),  "null");
    if (s.lightOk)    snprintf(lx, sizeof(lx), "%.0f", s.lux);         else snprintf(lx, sizeof(lx), "null");
    if (s.pressureOk) snprintf(pr, sizeof(pr), "%.2f", s.pressureKpa); else snprintf(pr, sizeof(pr), "null");

    // Tablica aktywnych alarmow
    char al[160];
    size_t pos = 0;
    al[pos++] = '[';
    bool first = true;
    for (uint8_t i = 0; i < ALARM_COUNT; i++) {
        if (!((s.alarmMask >> i) & 1u)) continue;
        const int w = snprintf(al + pos, sizeof(al) - pos, "%s\"%s\"", first ? "" : ",", AlarmManager::code((AlarmId)i));
        if (w < 0 || pos + (size_t)w >= sizeof(al) - 2) break;
        pos += (size_t)w;
        first = false;
    }
    al[pos++] = ']';
    al[pos] = '\0';

    return snprintf(buf, size,                  // JSON
        "{\"temp\":%s,\"tempOk\":%s,\"target\":%.2f,\"heater\":%s,"
        "\"tds\":%.0f,\"tdsMv\":%.0f,\"clarity\":%.0f,\"turbV\":%.2f,"
        "\"lux\":%s,\"lightOk\":%s,\"pressure\":%s,\"pressureOk\":%s,"
        "\"wifiRssi\":%d,\"uptime\":%lu,\"alarms\":%s}",
        t, s.tempOk ? "true" : "false", s.targetC, s.heaterOn ? "true" : "false",
        s.tdsPpm, s.tdsMv, s.clarityPct, s.turbidityV,
        lx, s.lightOk ? "true" : "false", pr, s.pressureOk ? "true" : "false",
        s.wifiRssi, (unsigned long)s.uptimeS, al);
}

}  // namespace WebApi
