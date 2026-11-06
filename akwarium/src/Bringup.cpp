#ifdef BRINGUP
#include "Bringup.h"
#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "SensorMath.h"
#include "BringupParser.h"
#include "TdsSensor.h"
#include "TurbiditySensor.h"
#include "PressureSensor.h"
#include "LightSensor.h"

static OneWire           oneWire(PIN_ONE_WIRE); // przypisanie czujników do pinów 
static DallasTemperature ds(&oneWire);
static TdsSensor         tds(PIN_ADC_TDS, FILTER_SAMPLES, 1.0f);   // k=1: kalibracje liczymy od zera
static TurbiditySensor   turbidity(PIN_ADC_TURBIDITY, FILTER_SAMPLES, TURBIDITY_DIVIDER, TURBIDITY_V_CLEAR);
static PressureSensor    pressure(PIN_PRESSURE_DATA, PIN_HX_SCK, FILTER_SAMPLES);
static LightSensor       light(BH1750_I2C_ADDR);

static bool     streaming = true;  
static uint32_t lastStreamMs = 0;
static String   rxLine;
static float    lastTempC = 25.0f;

static void relayWrite(bool on) { digitalWrite(PIN_RELAY_HEATER, (on == RELAY_ACTIVE_HIGH) ? HIGH : LOW); }

static float readTempBlocking() {
    ds.requestTemperatures();                 // w tym trybie blokowanie jest OK (brak sterowania grzalka)
    const float t = ds.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C) lastTempC = t;
    return t;
}

static void feedSensors(uint32_t ms) {        // daj filtrom czas na uzbieranie probek
    const uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        tds.update(lastTempC);
        turbidity.update();
        pressure.update();
        light.update();
        delay(2);
    }
}
 
static void printHelp() {  // wypisz liste komend
    Serial.println(F("--- KOMENDY ---"));
    Serial.println(F("help              - ta lista"));
    Serial.println(F("scan              - skan magistrali I2C i 1-Wire"));
    Serial.println(F("stream on|off     - wlacz/wylacz ciagly wydruk odczytow (co 1 s)"));
    Serial.println(F("relay on|off      - przekaznik (BEZ grzalki na 230 V przy tescie polaryzacji!)"));
    Serial.println(F("cal zero          - cisnienie: zapamietaj zero (pompka WYLACZONA, waz otwarty)"));
    Serial.println(F("cal press <kPa>   - cisnienie: czulosc przy znanym cisnieniu (np. slup wody 30 cm = 2.94 kPa)"));
    Serial.println(F("cal tds <ppm>     - TDS: wspolczynnik k wzgledem roztworu wzorcowego / miernika"));
    Serial.println(F("cal clear         - zmetnienie: napiecie w CZYSTEJ wodzie"));
}

static void doScan() {  // skanuj I2C i 1-Wire, wypisz wyniki na Serial
    Serial.println(F("--- SKAN I2C (SDA=21, SCL=22) ---"));
    int found = 0;
    for (uint8_t a = 1; a < 127; a++) {
        Wire.beginTransmission(a);
        if (Wire.endTransmission() == 0) {
            found++;
            Serial.printf("  znaleziono 0x%02X", a);
            if (a == 0x3C || a == 0x3D) Serial.print("  <- wyswietlacz OLED SSD1306");
            if (a == 0x23 || a == 0x5C) Serial.print("  <- czujnik swiatla BH1750");
            Serial.println();
        }
    }
    if (found == 0) Serial.println(F("  BRAK urzadzen I2C (sprawdz zasilanie 3V3, GND, SDA, SCL)"));
    Serial.printf("  oczekiwane: OLED 0x%02X, BH1750 0x%02X\n", OLED_I2C_ADDR, BH1750_I2C_ADDR);

    Serial.println(F("--- SKAN 1-Wire (DS18B20, GPIO4) ---")); // skanuj magistralę 1-Wire
    ds.begin();
    const int n = ds.getDeviceCount();
    Serial.printf("  liczba czujnikow DS18B20: %d\n", n);
    for (int i = 0; i < n; i++) {
        DeviceAddress addr;
        if (ds.getAddress(addr, i)) {
            Serial.print("  adres ROM: ");
            for (int b = 0; b < 8; b++) Serial.printf("%02X", addr[b]);
            Serial.println();
        }
    }
    if (n == 0) Serial.println(F("  BRAK czujnika (sprawdz rezystor 4,7 kohm DQ-3V3, przewody, zasilanie)"));
}

static void printReadings() { // wypisz odczyty z czujnikow na Serial
    const float t = readTempBlocking();
    if (t == DEVICE_DISCONNECTED_C) Serial.println(F("T: BRAK ODCZYTU (-127)"));
    else Serial.printf("T = %.2f C", t);
    Serial.printf(" | TDS: %.0f mV -> %.0f ppm (k=1)", tds.getVoltageMv(), tds.getTdsPpm());
    Serial.printf(" | zmetnienie: %.2f V czujnika, przejrzystosc %.0f%%", turbidity.getSensorVoltage(), turbidity.getClarityPercent());
    Serial.printf(" | cisnienie: raw %.0f -> %.2f kPa %s", pressure.getRaw(), pressure.getKpa(), pressure.isOk() ? "" : "[BRAK ODCZYTU]");
    Serial.printf(" | swiatlo: %.0f lx %s\n", light.getLux(), light.isOk() ? "" : "[BRAK CZUJNIKA]");
}

static void handle(const BringupParsed& p) { // wykonaj komendę z Serial
    switch (p.cmd) {
    case BringupCmd::NONE: break;
    case BringupCmd::HELP: printHelp(); break;
    case BringupCmd::SCAN: doScan(); break;
    case BringupCmd::STREAM_ON:  streaming = true;  Serial.println(F("stream: ON")); break;
    case BringupCmd::STREAM_OFF: streaming = false; Serial.println(F("stream: OFF")); break;

    case BringupCmd::RELAY_ON: // wylacz grzalkę, sprawdz polaryzacje przekaznika (BEZ grzalki na 230 V)
        relayWrite(true);
        Serial.printf("Przekaznik: ON -> pin GPIO%d = %s (RELAY_ACTIVE_HIGH=%s)\n", PIN_RELAY_HEATER,
                      RELAY_ACTIVE_HIGH ? "HIGH" : "LOW", RELAY_ACTIVE_HIGH ? "true" : "false");
        Serial.println(F("Sprawdz: czy przekaznik kliknal i zapalila sie dioda na module? Jesli NIE, a przy 'relay off' klika -> odwroc RELAY_ACTIVE_HIGH w config.h"));
        break;
    case BringupCmd::RELAY_OFF: // wylacz grzalkę, sprawdz polaryzacje przekaznika (BEZ grzalki na 230 V)
        relayWrite(false);
        Serial.printf("Przekaznik: OFF -> pin GPIO%d = %s\n", PIN_RELAY_HEATER, RELAY_ACTIVE_HIGH ? "LOW" : "HIGH");
        Serial.println(F("KRYTYCZNE: po 'relay off' przekaznik MUSI byc wylaczony (brak klikniecia/diody). Jesli zostaje wlaczony -> modul nie nadaje sie do 3,3 V"));
        break;

    case BringupCmd::CAL_ZERO: { // zapisz surowy odczyt cisnienia przy pompce WYLACZONEJ i wazie OTWARTYM
        Serial.println(F("Zbieram probki (3 s)... pompka ma byc WYLACZONA, waz otwarty"));
        feedSensors(3000);
        Serial.printf("WYNIK: PRESSURE_RAW_ZERO = %.0f.0f;   (wpisz w config.h, sekcja #else)\n", pressure.getRaw());
        break; }
    case BringupCmd::CAL_PRESS: { // zapisz surowy odczyt cisnienia przy znanym cisnieniu (np. slup wody 30 cm = 2.94 kPa)
        Serial.printf("Zbieram probki (3 s) dla znanego cisnienia %.2f kPa...\n", p.arg);
        feedSensors(3000);
        Serial.printf("Surowy odczyt: %.0f. Uzyj wartosci ZERO z 'cal zero'.\n", pressure.getRaw());
        Serial.println(F("Wzor: PER_KPA = (raw - ZERO) / kPa. Wpisz raw ponizej do kalkulatora ponizej:"));
        Serial.printf("  jesli ZERO=0 -> PRESSURE_RAW_PER_KPA = %.1f\n", SensorMath::pressureRawPerKpa(pressure.getRaw(), 0.0f, p.arg));
        Serial.printf("  (inna wartosc ZERO: (%.0f - ZERO) / %.2f)\n", pressure.getRaw(), p.arg);
        break; }
    case BringupCmd::CAL_TDS: { // kalibracja TDS: podaj wzorcowe stężenie w ppm, policz wspolczynnik k
        readTempBlocking();
        Serial.printf("Zbieram probki (3 s), temperatura wody %.1f C, wzorzec %.0f ppm...\n", lastTempC, p.arg);
        feedSensors(3000);
        const float measured = SensorMath::tdsFromVoltage(tds.getVoltageMv() / 1000.0f, lastTempC, 1.0f);
        Serial.printf("Zmierzone (k=1): %.0f ppm\n", measured);
        Serial.printf("WYNIK: TDS_K_VALUE = %.3ff;   (wpisz w config.h)\n", SensorMath::tdsKFromReference(p.arg, measured));
        break; }
    case BringupCmd::CAL_CLEAR: { // kalibracja zmetnienia: podaj napiecie w czystej wodzie, policz przejrzystosc
        Serial.println(F("Zbieram probki (3 s) - czujnik w CZYSTEJ wodzie..."));
        feedSensors(3000);
        Serial.printf("WYNIK: TURBIDITY_V_CLEAR = %.2ff;   (wpisz w config.h)\n", turbidity.getSensorVoltage());
        break; }

    case BringupCmd::BAD_ARG: Serial.println(F("Bledny lub brakujacy argument. Wpisz 'help'.")); break;
    case BringupCmd::UNKNOWN: Serial.println(F("Nieznana komenda. Wpisz 'help'.")); break;
    }
}

void bringupSetup() { // inicjalizacja czujnikow i Serial, wypisz help i skanuj I2C/1-Wire
    Serial.begin(115200);
    delay(300);
    pinMode(PIN_RELAY_HEATER, OUTPUT);
    relayWrite(false);                              // grzalka zawsze OFF na starcie
    Serial.println(F("\n=== AKWARIUM: FIRMWARE DIAGNOSTYCZNY (bringup) ==="));
    Serial.println(F("Ten tryb NIE steruje grzalka. Przekaznik jest wylaczony."));

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(I2C_CLOCK_HZ);
    tds.begin(); turbidity.begin(); pressure.begin(); light.begin();

    printHelp();
    doScan();
    Serial.println(F("--- Odczyty co 1 s (wpisz 'stream off' aby wylaczyc) ---"));
}

void bringupLoop() { // odczyt czujnikow, obsluga Serial, wyswietlanie odczytow co 1 s
    tds.update(lastTempC);
    turbidity.update();
    pressure.update();
    light.update();

    while (Serial.available()) {
        const char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (rxLine.length() > 0) { handle(parseBringupCommand(rxLine.c_str())); rxLine = ""; }
        } else if (rxLine.length() < 40) {
            rxLine += c;
        }
    }

    if (streaming && millis() - lastStreamMs >= 1000) {
        lastStreamMs = millis();
        printReadings();
    }
}
#endif  // BRINGUP
