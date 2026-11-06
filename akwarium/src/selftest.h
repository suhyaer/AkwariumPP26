#pragma once
// Samotest filtra MovingAverage i funkcji przeliczeniowych czujnikow uruchamiany NA ESP32
#include <Arduino.h>
#include <math.h>
#include "MovingAverage.h"
#include "SensorMath.h"
#include "WebApi.h"
#include "Debounce.h"
#include "AlarmManager.h"
#include "BringupParser.h"
#include "TemperatureManager.h"
#include "config.h"
#include <string.h>

namespace selftest { // funkcje selftestu

static int passed = 0;
static int failed = 0;

inline void checkTrue(const char* name, bool cond) {
    if (cond) passed++; else failed++;
    Serial.printf("[%s] %s\n", cond ? "PASS" : "FAIL", name);
}


// Podaje AlarmManagerowi ten sam stan co 1 s przez 'durationMs' (czas symulowany, bez czekania).
inline uint32_t runAlarms(AlarmManager& am, const SystemStatus& st, uint32_t startMs, uint32_t durationMs) {
    uint32_t t = startMs;
    for (uint32_t elapsed = 0; elapsed <= durationMs; elapsed += 1000) { t = startMs + elapsed; am.update(st, t); }
    return t;
}
inline SystemStatus healthyStatus() { // wszystkie czujniki ok
    SystemStatus st = {};
    st.tempC = 25.0f; st.tempOk = true; st.targetC = 25.0f;
    st.tdsPpm = 300.0f; st.clarityPct = 90.0f; st.lightOk = true;
    st.pressureKpa = 6.0f; st.pressureOk = true;
    return st;
}

// Wartosc referencyjna 
static const float TDS_35C = 307.011f;

inline void check(const char* name, float expected, float actual, float tol = 0.001f) {
    const bool ok = fabsf(expected - actual) <= tol;
    if (ok) passed++; else failed++;
    Serial.printf("[%s] %s (oczekiwano %.3f, jest %.3f)\n", ok ? "PASS" : "FAIL", name, expected, actual);
}

inline void run() {
    Serial.println("=== SAMOTEST: filtr, przeliczenia, API WWW, walidacja temperatury ==="); // testy 

    {   MovingAverage f(10);
        check("pierwsza probka zwraca sama siebie", 5.0f, f.addSample(5.0f)); }

    {   MovingAverage f(10);
        f.addSample(1.0f); f.addSample(2.0f);
        check("niepelny bufor: srednia z liczby probek", 2.0f, f.addSample(3.0f)); }

    {   MovingAverage f(3);
        f.addSample(1.0f); f.addSample(2.0f); f.addSample(3.0f);
        check("okno sie przesuwa (1)", 3.0f, f.addSample(4.0f));
        check("okno sie przesuwa (2)", 4.0f, f.addSample(5.0f)); }

    {   MovingAverage f(10);
        float r = 0;
        for (int i = 0; i < 50; i++) r = f.addSample(7.5f);
        check("stale wejscie -> stale wyjscie", 7.5f, r); }

    {   MovingAverage f(10);
        for (int i = 0; i < 10; i++) f.addSample(100.0f);
        check("skok 1000 na tle 100 oslabiony do 190", 190.0f, f.addSample(1000.0f)); }

    {   MovingAverage f(5);
        for (int i = 0; i < 5; i++) f.addSample(100.0f);
        f.reset();
        check("reset czysci bufor", 10.0f, f.addSample(10.0f)); }

    {   MovingAverage f(0);
        check("okno 0 -> okno 1 (1)", 4.0f, f.addSample(4.0f));
        check("okno 0 -> okno 1 (2)", 6.0f, f.addSample(6.0f)); }


    // SensorMath: TDS 
    check("TDS 0 V = 0 ppm", 0.0f, SensorMath::tdsFromVoltage(0.0f, 25.0f));
    check("TDS 1.0 V @25C = 367.475 ppm", 367.475f, SensorMath::tdsFromVoltage(1.0f, 25.0f), 0.01f);
    check("TDS 1.0 V @35C (kompensacja) = TDS_35C", TDS_35C, SensorMath::tdsFromVoltage(1.0f, 35.0f), 0.05f);
    check("TDS wspolczynnik k=2 podwaja wynik", 734.95f, SensorMath::tdsFromVoltage(1.0f, 25.0f, 2.0f), 0.02f);

    // SensorMath: zmetnienie 
    check("przejrzystosc = 100% przy V_czysta", 100.0f, SensorMath::turbidityClarityPercent(4.1f, 4.1f));
    check("przejrzystosc = 50% przy polowie V", 50.0f, SensorMath::turbidityClarityPercent(2.05f, 4.1f));
    check("przejrzystosc obcieta do 100%", 100.0f, SensorMath::turbidityClarityPercent(5.0f, 4.1f));
    check("przejrzystosc obcieta do 0%", 0.0f, SensorMath::turbidityClarityPercent(-1.0f, 4.1f));

    // SensorMath: HX710B 
    check("HX 0x000001 = 1", 1.0f, (float)SensorMath::signExtend24(0x000001u));
    check("HX 0x7FFFFF = +8388607", 8388607.0f, (float)SensorMath::signExtend24(0x7FFFFFu));
    check("HX 0x800000 = -8388608", -8388608.0f, (float)SensorMath::signExtend24(0x800000u));
    check("HX 0xFFFFFF = -1", -1.0f, (float)SensorMath::signExtend24(0xFFFFFFu));

    // SensorMath: cisnienie 
    check("kPa = (raw-zero)/per_kPa (1)", 10.0f, SensorMath::pressureKpa(1000.0f, 0.0f, 100.0f));
    check("kPa = (raw-zero)/per_kPa (2)", 4.0f, SensorMath::pressureKpa(500.0f, 100.0f, 100.0f));


    // WebApi: parsowanie parametru temperatury 
    {   float v = 0;
        checkTrue("parse \"26.5\" -> 26.5", WebApi::parseTargetParam("26.5", v) && fabsf(v - 26.5f) < 0.001f);
        checkTrue("parse \"26,5\" (przecinek) -> 26.5", WebApi::parseTargetParam("26,5", v) && fabsf(v - 26.5f) < 0.001f);
        checkTrue("parse \" 27 \" (spacje) -> 27", WebApi::parseTargetParam(" 27 ", v) && fabsf(v - 27.0f) < 0.001f);
        checkTrue("parse \"abc\" odrzucone", !WebApi::parseTargetParam("abc", v));
        checkTrue("parse \"\" odrzucone", !WebApi::parseTargetParam("", v));
        checkTrue("parse \"26.5x\" odrzucone", !WebApi::parseTargetParam("26.5x", v));
        checkTrue("parse \"nan\" odrzucone", !WebApi::parseTargetParam("nan", v));
        checkTrue("parse \"inf\" odrzucone", !WebApi::parseTargetParam("inf", v));
        checkTrue("parse nullptr odrzucone", !WebApi::parseTargetParam(nullptr, v));
    }

    // WebApi: JSON ze stanem 
    {   SystemStatus st = {};
        st.tempC = 24.5f; st.tempOk = true; st.targetC = 25.0f; st.heaterOn = true;
        st.tdsPpm = 412.0f; st.tdsMv = 1230.0f; st.clarityPct = 87.0f; st.turbidityV = 3.57f;
        st.lux = 5200.0f; st.lightOk = true; st.pressureKpa = 5.2f; st.pressureOk = true;
        st.wifiRssi = -58; st.uptimeS = 3723;
        char buf[512];
        int n = WebApi::buildStatusJson(st, buf, sizeof(buf));
        checkTrue("JSON miesci sie w buforze", n > 0 && n < (int)sizeof(buf));
        checkTrue("JSON zaczyna sie od { i konczy }", buf[0] == '{' && buf[n - 1] == '}');
        checkTrue("JSON: temp=24.50", strstr(buf, "\"temp\":24.50") != nullptr);
        checkTrue("JSON: heater=true", strstr(buf, "\"heater\":true") != nullptr);
        checkTrue("JSON: uptime=3723", strstr(buf, "\"uptime\":3723") != nullptr);
        st.tempOk = false; st.lightOk = false; st.pressureOk = false;
        WebApi::buildStatusJson(st, buf, sizeof(buf));
        checkTrue("JSON: brak odczytu temp -> null", strstr(buf, "\"temp\":null") != nullptr);
        checkTrue("JSON: brak odczytu swiatla -> null", strstr(buf, "\"lux\":null") != nullptr);
        checkTrue("JSON: brak odczytu cisnienia -> null", strstr(buf, "\"pressure\":null") != nullptr);
        char tiny[16];
        int m = WebApi::buildStatusJson(st, tiny, sizeof(tiny));
        checkTrue("JSON: za maly bufor wykrywalny (n >= rozmiar)", m >= (int)sizeof(tiny));
    }

    // TemperatureManager: walidacja temperatury zadanej (zastepuje komende `set`) 
    // UWAGA: na prawdziwym ESP32 ten test zapisuje do NVS (kilka zapisow); na koncu przywraca wartosc.
    {   TemperatureManager tm(PIN_ONE_WIRE, PIN_RELAY_HEATER, DEFAULT_TEMP_SETPOINT, TEMP_HYSTERESIS, RELAY_ACTIVE_HIGH);
        tm.begin();
        const float orig = tm.getTargetTemp();
        checkTrue("set 40 odrzucone", !tm.setTargetTemperature(40.0f));
        check("po odrzuconym set zadana bez zmian", orig, tm.getTargetTemp());
        checkTrue("set NaN odrzucone", !tm.setTargetTemperature(NAN));
        checkTrue("set 14.9 odrzucone (ponizej 15)", !tm.setTargetTemperature(14.9f));
        checkTrue("set 32.1 odrzucone (powyzej 32)", !tm.setTargetTemperature(32.1f));
        checkTrue("set 15.0 przyjete (granica)", tm.setTargetTemperature(15.0f));
        check("zadana = 15.0", 15.0f, tm.getTargetTemp());
        checkTrue("set 32.0 przyjete (granica)", tm.setTargetTemperature(32.0f));
        check("zadana = 32.0", 32.0f, tm.getTargetTemp());
        checkTrue("set 26.5 przyjete", tm.setTargetTemperature(26.5f));
        check("zadana = 26.5", 26.5f, tm.getTargetTemp());
        tm.setTargetTemperature(orig);   // przywrocenie wartosci sprzed testu
    }


    // Debounce (czas symulowany) 
    {   Debounce d;
        checkTrue("debounce: start nieaktywny", !d.update(false, 0, 1000, 500));
        d = Debounce();
        checkTrue("debounce: 999 ms jeszcze nie", !(d.update(true, 0, 1000, 500), d.update(true, 999, 1000, 500)));
        checkTrue("debounce: 1000 ms wlacza", d.update(true, 1000, 1000, 500));
        checkTrue("debounce: znikniecie 100 ms nie gasi", d.update(false, 1100, 1000, 500));
        checkTrue("debounce: znikniecie 500 ms gasi", !d.update(false, 1600, 1000, 500));
        Debounce g;   // przerywa odliczanie
        g.update(true, 0, 1000, 500); g.update(false, 500, 1000, 500); g.update(true, 600, 1000, 500);
        checkTrue("debounce: po glitchu odliczanie od nowa (1599)", !g.update(true, 1599, 1000, 500));
        checkTrue("debounce: po glitchu wlacza o 1600", g.update(true, 1600, 1000, 500));
        Debounce w;   // przepelnienie millis() (po ok. 49 dniach)
        w.update(true, 0xFFFFFF00u, 1000, 500);
        checkTrue("debounce: dziala przez przepelnienie licznika", w.update(true, 0xFFFFFF00u + 1000u, 1000, 500));
    }

    // AlarmManager (czas symulowany) 
    {   AlarmManager am; SystemStatus st = healthyStatus();
        runAlarms(am, st, 0, 7200000UL);   // 2 godziny zdrowego stanu
        checkTrue("alarmy: zdrowy stan = brak alarmow", am.getMask() == 0);
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.pressureKpa = 0.2f; // za niskie cisnienie
        uint32_t t = runAlarms(am, st, 0, ALARM_AERATION_DELAY_MS - 1000);
        checkTrue("napowietrzanie: przed opoznieniem brak alarmu", !am.isActive(ALARM_AERATION_LOW));
        am.update(st, t + 1000);
        checkTrue("napowietrzanie: niskie cisnienie -> alarm po opoznieniu", am.isActive(ALARM_AERATION_LOW));
        st.pressureKpa = 6.0f;
        t = runAlarms(am, st, t + 2000, ALARM_CLEAR_DELAY_MS + 1000);
        checkTrue("napowietrzanie: alarm gasnie po powrocie do normy", !am.isActive(ALARM_AERATION_LOW));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.pressureKpa = 30.0f; // za wysokie cisnienie
        runAlarms(am, st, 0, ALARM_AERATION_DELAY_MS + 1000);
        checkTrue("napowietrzanie: wysokie cisnienie -> zator", am.isActive(ALARM_AERATION_HIGH) && !am.isActive(ALARM_AERATION_LOW));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.pressureOk = false; st.pressureKpa = 0.0f; // awaria czujnika cisnienia
        runAlarms(am, st, 0, ALARM_PRESSURE_SENSOR_DELAY_MS + ALARM_AERATION_DELAY_MS + 1000);
        checkTrue("awaria czujnika cisnienia -> alarm czujnika", am.isActive(ALARM_PRESSURE_SENSOR));
        checkTrue("awaria czujnika cisnienia NIE daje falszywego 'brak napowietrzania'", !am.isActive(ALARM_AERATION_LOW));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.tempOk = false; st.tempC = -127.0f; // awaria czujnika temperatury
        runAlarms(am, st, 0, ALARM_TEMP_SENSOR_DELAY_MS + 1000);
        checkTrue("brak czujnika temperatury -> alarm", am.isActive(ALARM_TEMP_SENSOR));
        checkTrue("brak czujnika temperatury NIE daje 'temp. za niska'", !am.isActive(ALARM_TEMP_LOW));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.tempC = 27.5f;   // Tzad 25 + 2.5
        runAlarms(am, st, 0, ALARM_TEMP_HIGH_DELAY_MS + 1000);
        checkTrue("temperatura za wysoka -> alarm", am.isActive(ALARM_TEMP_HIGH));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.tempC = 22.5f;   // Tzad 25 - 2.5
        uint32_t t = runAlarms(am, st, 0, ALARM_TEMP_LOW_DELAY_MS - 2000);
        checkTrue("temp. za niska: przed uplywem czasu brak alarmu (nagrzewanie trwa)", !am.isActive(ALARM_TEMP_LOW));
        runAlarms(am, st, t + 1000, 3000);
        checkTrue("temp. za niska: po uplywie czasu alarm", am.isActive(ALARM_TEMP_LOW));
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.clarityPct = 40.0f; st.tdsPpm = 750.0f;
        runAlarms(am, st, 0, ALARM_TURBID_DELAY_MS + ALARM_TDS_DELAY_MS + 1000);
        checkTrue("mętna woda -> alarm", am.isActive(ALARM_TURBID));
        checkTrue("wysokie TDS -> alarm", am.isActive(ALARM_TDS_HIGH));
        checkTrue("licznik alarmow = 2", am.count() == 2);
    }
    {   AlarmManager am; SystemStatus st = healthyStatus(); st.lightOk = false;
        runAlarms(am, st, 0, ALARM_LIGHT_SENSOR_DELAY_MS + 1000);
        checkTrue("brak czujnika swiatla -> alarm", am.isActive(ALARM_LIGHT_SENSOR));
    }

        //  JSON z alarmami 
    {   SystemStatus st = healthyStatus();
        st.alarmMask = (1u << ALARM_AERATION_LOW) | (1u << ALARM_TURBID);
        char buf[768];
        int n = WebApi::buildStatusJson(st, buf, sizeof(buf));
        checkTrue("JSON: tablica alarmow z dwoma kodami",
                  n > 0 && strstr(buf, "\"alarms\":[\"aeration_low\",\"turbid\"]") != nullptr);
        st.alarmMask = 0;
        WebApi::buildStatusJson(st, buf, sizeof(buf));
        checkTrue("JSON: brak alarmow -> pusta tablica", strstr(buf, "\"alarms\":[]") != nullptr);
        st.alarmMask = (1u << ALARM_COUNT) - 1;   // wszystkie alarmy naraz - najgorszy przypadek rozmiaru
        n = WebApi::buildStatusJson(st, buf, sizeof(buf));
        checkTrue("JSON: wszystkie alarmy miesza sie w buforze 768 B",
                  n > 0 && n < (int)sizeof(buf) && strstr(buf, "\"light_sensor\"") != nullptr);
    }


    //  Kalibracja: matematyka 
    check("k TDS: wzorzec 342, zmierzono 300 -> 1.14", 1.14f, SensorMath::tdsKFromReference(342.0f, 300.0f), 0.001f);
    check("k TDS: zmierzono 0 -> 0 (bez dzielenia przez zero)", 0.0f, SensorMath::tdsKFromReference(342.0f, 0.0f));
    check("czulosc cisnienia: (2294-2000)/2.94 = 100", 100.0f, SensorMath::pressureRawPerKpa(2294.0f, 2000.0f, 2.94f), 0.01f);
    check("czulosc cisnienia moze byc ujemna", -100.0f, SensorMath::pressureRawPerKpa(1706.0f, 2000.0f, 2.94f), 0.01f);
    check("slup wody 30 cm = 2.94 kPa", 2.942f, SensorMath::waterColumnKpa(30.0f), 0.005f);

    // komendy bringup
    {   BringupParsed p;
        checkTrue("cmd: pusta linia = NONE", parseBringupCommand("").cmd == BringupCmd::NONE);
        checkTrue("cmd: help", parseBringupCommand("help").cmd == BringupCmd::HELP);
        checkTrue("cmd: SCAN (wielkie litery)", parseBringupCommand("SCAN").cmd == BringupCmd::SCAN);
        checkTrue("cmd: stream on", parseBringupCommand("stream on").cmd == BringupCmd::STREAM_ON);
        checkTrue("cmd: stream off", parseBringupCommand("stream off").cmd == BringupCmd::STREAM_OFF);
        checkTrue("cmd: stream (bez argumentu) = BAD_ARG", parseBringupCommand("stream").cmd == BringupCmd::BAD_ARG);
        checkTrue("cmd: relay on", parseBringupCommand("relay on").cmd == BringupCmd::RELAY_ON);
        checkTrue("cmd: relay off", parseBringupCommand("relay off").cmd == BringupCmd::RELAY_OFF);
        checkTrue("cmd: cal zero", parseBringupCommand("cal zero").cmd == BringupCmd::CAL_ZERO);
        checkTrue("cmd: cal clear", parseBringupCommand("cal clear").cmd == BringupCmd::CAL_CLEAR);
        p = parseBringupCommand("cal press 2.94");
        checkTrue("cmd: cal press 2.94", p.cmd == BringupCmd::CAL_PRESS && fabsf(p.arg - 2.94f) < 0.001f);
        p = parseBringupCommand("cal tds 342");
        checkTrue("cmd: cal tds 342", p.cmd == BringupCmd::CAL_TDS && fabsf(p.arg - 342.0f) < 0.001f);
        checkTrue("cmd: cal tds (bez liczby) = BAD_ARG", parseBringupCommand("cal tds").cmd == BringupCmd::BAD_ARG);
        checkTrue("cmd: cal tds abc = BAD_ARG", parseBringupCommand("cal tds abc").cmd == BringupCmd::BAD_ARG);
        checkTrue("cmd: cal tds 0 = BAD_ARG (wzorzec musi byc > 0)", parseBringupCommand("cal tds 0").cmd == BringupCmd::BAD_ARG);
        checkTrue("cmd: nieznana = UNKNOWN", parseBringupCommand("foo").cmd == BringupCmd::UNKNOWN);
        checkTrue("cmd: nullptr = NONE", parseBringupCommand(nullptr).cmd == BringupCmd::NONE);
    }

    Serial.printf("=== WYNIK: %d PASS, %d FAIL ===\n", passed, failed);
}

}  
