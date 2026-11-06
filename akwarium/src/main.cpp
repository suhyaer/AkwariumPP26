#include <Arduino.h>

#if defined(BRINGUP)
// Firmware diagnostyczny do pierwszego uruchomienia sprzetu
#include "Bringup.h"
void setup() { bringupSetup(); }
void loop()  { bringupLoop(); }

#elif defined(SELFTEST)
// Tryb samotestu (srodowisko "selftest"): testy filtra i funkcji przeliczeniowych
void setup() {
    Serial.begin(115200);
    delay(300);
    selftest::run();
}
void loop() {}

#else
// --- Normalny firmware ---
#include "config.h"
#include "TemperatureManager.h"
#include "TdsSensor.h"
#include "TurbiditySensor.h"
#include "PressureSensor.h"
#include "LightSensor.h"
#include "DisplayManager.h"
#include "WifiManager.h"
#include "WebInterface.h"
#include "AlarmManager.h"
#include "Watchdog.h"
#include <Wire.h>

// Dane sieci w symulatorze otwarta siec Wokwi-GUEST na gotowym projekcie plik secrets.h
#ifdef SIMULATION
  #define WIFI_SSID_USED "Wokwi-GUEST"
  #define WIFI_PASS_USED ""
  #define WIFI_CHANNEL_USED 6
#else // wlasciwe urzadzenie: wlasne dane sieci w pliku src/secrets.h (skopiuj src/secrets.example.h jako src/secrets.h i wpisz dane)
  #if __has_include("secrets.h")
    #include "secrets.h"
  #else
    #error "Brak src/secrets.h - skopiuj src/secrets.example.h jako src/secrets.h i wpisz dane sieci Wi-Fi"
  #endif
  #define WIFI_SSID_USED WIFI_SSID
  #define WIFI_PASS_USED WIFI_PASSWORD
  #define WIFI_CHANNEL_USED 0
#endif

TemperatureManager tempMgr(PIN_ONE_WIRE, PIN_RELAY_HEATER,  // Tworzenie obiektów zarzadzajacych czujnikami i urzadzeniami
                           DEFAULT_TEMP_SETPOINT, TEMP_HYSTERESIS,
                           RELAY_ACTIVE_HIGH);
TdsSensor       tds(PIN_ADC_TDS, FILTER_SAMPLES, TDS_K_VALUE); 
TurbiditySensor turbidity(PIN_ADC_TURBIDITY, FILTER_SAMPLES, TURBIDITY_DIVIDER, TURBIDITY_V_CLEAR);
PressureSensor  pressure(PIN_PRESSURE_DATA, PIN_HX_SCK, FILTER_SAMPLES);
LightSensor     light(BH1750_I2C_ADDR);
DisplayManager  oled(OLED_I2C_ADDR, DISPLAY_PAGE_MS);
WifiManager     wifi(WIFI_SSID_USED, WIFI_PASS_USED, WIFI_CHANNEL_USED);
WebInterface    web(tempMgr);
AlarmManager    alarms;

static uint32_t lastPrintMs = 0;  // Zmienne pomocnicze do wypisywania stanu w konsoli co 2 s
static uint16_t lastAlarmMask = 0;
static String rxLine;

// Prosty interpreter komend z monitora portu (do testow):  set 26.5
static void handleSerialCommands() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {          // Enter moze wyslac \r, \n lub oba
            rxLine.trim();
            if (rxLine.length() == 0) continue;
            if (rxLine.startsWith("set ")) {
                float v = rxLine.substring(4).toFloat();   // uwaga: kropka, nie przecinek do ustalenia wartosci
                bool ok = tempMgr.setTargetTemperature(v);
                Serial.printf("Komenda: set %.2f -> %s\n", v, ok ? "przyjeta" : "ODRZUCONA");
            } else {
                Serial.println("Nieznana komenda. Uzyj: set <temperatura>, np. set 26.5");
            }
            rxLine = "";
        } else if (rxLine.length() < 32) {
            rxLine += c;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("=== Akwarium: v1.0 ==="); //start logu w konsoli 
    tempMgr.begin();
    tds.begin();
    turbidity.begin();
    pressure.begin();

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);   // jedna magistrala I2C dla OLED i BH1750
    Wire.setClock(I2C_CLOCK_HZ);
    light.begin();
    oled.begin();

    wifi.begin();    // laczenie w tle
    web.begin();

    Serial.printf("Powod ostatniego restartu: %s\n", resetReasonText());
    watchdogBegin(WATCHDOG_TIMEOUT_S);   // od teraz petla musi wolac watchdogFeed() co najmniej co 10 s
    Serial.printf("Watchdog wlaczony (%u s)\n", (unsigned)WATCHDOG_TIMEOUT_S);
}

void loop() {
    watchdogFeed();                            // odp watchdoga (ESP32 resetuje sie po WATCHDOG_TIMEOUT_S s braku wywolania)

#ifdef WDT_TEST
    if (millis() > 20000) {                    // TEST: celowe zawieszenie petli po 20 s (tylko do testow)
        Serial.println("TEST WDT: celowe zawieszenie petli - za ok. 10 s powinien nastapic restart");
        while (true) {}
    }
#endif

    tempMgr.update();                          // wszystkie moduly sa nieblokujace
    tds.update(tempMgr.getCurrentTemp());      // TDS potrzebuje temperatury do kompensacji
    turbidity.update();
    pressure.update();
    light.update();
    handleSerialCommands();

    wifi.update();

    SystemStatus st = {};   // Pozyskujemy aktualny stan wszystkich czujnikow i urzadzen do wyswietlenia na OLED i w interfejsie WWW
    st.tempC = tempMgr.getCurrentTemp();   st.tempOk = tempMgr.isSensorOk();
    st.targetC = tempMgr.getTargetTemp();  st.heaterOn = tempMgr.getHeaterState();
    st.tdsPpm = tds.getTdsPpm();           st.tdsMv = tds.getVoltageMv();
    st.clarityPct = turbidity.getClarityPercent();  st.turbidityV = turbidity.getSensorVoltage();
    st.lux = light.getLux();               st.lightOk = light.isOk();
    st.pressureKpa = pressure.getKpa();    st.pressureOk = pressure.isOk();
    st.wifiConnected = wifi.isConnected(); st.wifiRssi = wifi.rssi();
    strncpy(st.ip, wifi.ip(), sizeof(st.ip) - 1);
    st.uptimeS = millis() / 1000;

    alarms.update(st, millis());
    st.alarmMask = alarms.getMask();

    if (st.alarmMask != lastAlarmMask) {       // sprawdzenie czy nastapila zmiana stanu alarmow i wypisanie w konsoli
        for (uint8_t i = 0; i < ALARM_COUNT; i++) {
            const bool now = (st.alarmMask >> i) & 1u, was = (lastAlarmMask >> i) & 1u;
            if (now != was) Serial.printf("ALARM %s: %s\n", now ? "WLACZONY" : "zakonczony", AlarmManager::name((AlarmId)i));
        }
        lastAlarmMask = st.alarmMask;
    }

    oled.update(st);
    web.update(st);

    if (millis() - lastPrintMs >= 2000) {          // co 2 s wypisz stan w konsoli
        lastPrintMs = millis();
        Serial.printf("T=%.2f C | zadana=%.2f C | grzalka=%s | czujnik=%s\n",   // wypisz dane z czujników
                      tempMgr.getCurrentTemp(), tempMgr.getTargetTemp(),
                      tempMgr.getHeaterState() ? "ON" : "OFF",
                      tempMgr.isSensorOk() ? "OK" : "BLAD");
        Serial.printf("   TDS=%.0f ppm (%.0f mV) | przejrzystosc=%.0f%% (%.2f V) | cisnienie=%.2f kPa (raw %.0f) %s\n",
                      tds.getTdsPpm(), tds.getVoltageMv(),
                      turbidity.getClarityPercent(), turbidity.getSensorVoltage(),
                      pressure.getKpa(), pressure.getRaw(),
                      pressure.isOk() ? "" : "[BRAK ODCZYTU]");
        Serial.printf("   swiatlo=%.0f lx %s | wifi=%s (%d dBm)\n", light.getLux(),
                      light.isOk() ? "" : "[BRAK CZUJNIKA]",
                      wifi.isConnected() ? "OK" : "BRAK", wifi.rssi());
    }
}

#endif  // SELFTEST
