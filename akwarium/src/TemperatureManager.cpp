#include "TemperatureManager.h"
#include <Arduino.h>
#include <math.h>

TemperatureManager::TemperatureManager(int oneWirePin, int relayPin, float defaultTarget,
                                       float hysteresisHalfBand, bool relayActiveHigh) // inicjalizacja obiektu
    : oneWire(oneWirePin), sensor(&oneWire), 
      relayPin(relayPin), relayActiveHigh(relayActiveHigh),
      defaultTarget(defaultTarget), targetTemp(defaultTarget),
      currentTemp(DEVICE_DISCONNECTED_C), hysteresis(fabsf(hysteresisHalfBand)),
      heaterState(false), sensorOk(false),
      conversionPending(false), lastRequestMs(0), lastErrorLogMs(0), conversionTimeMs(800)
{
    // Domyslna wartosc musi miescic sie w dozwolonym zakresie
    this->defaultTarget = fminf(fmaxf(defaultTarget, MIN_TARGET_TEMP), MAX_TARGET_TEMP);
    this->targetTemp    = this->defaultTarget;
}

bool TemperatureManager::isTargetValid(float t) { // czy temperatura zadana miesci sie w dozwolonym zakresie
    return !isnan(t) && t >= MIN_TARGET_TEMP && t <= MAX_TARGET_TEMP;
}

void TemperatureManager::begin() { // inicjalizacja sprzetu i wczytanie danych z NVS
    // Pin przekaznika: od razu ustaw stan "wylaczony"
    pinMode(relayPin, OUTPUT);
    turnHeaterOff();

    // Czujnik DS18B20 - odczyt nieblokujacy, 12 bitow
    sensor.begin();
    sensor.setWaitForConversion(false);
    sensor.setResolution(12);
    conversionTimeMs = sensor.millisToWaitForConversion(12) + 50;  // ok. 750 ms + zapas

    if (sensor.getDeviceCount() == 0) {
        Serial.println("BLAD: Nie wykryto zadnego czujnika DS18B20 na magistrali 1-Wire!");
    }

    // Pierwszy pomiar od razu przy pierwszym update()
    lastRequestMs = millis() - READ_INTERVAL_MS;
    lastErrorLogMs = millis() - ERROR_LOG_INTERVAL_MS;  // pierwszy blad zalogowany od razu

    // NVS: wczytaj zapisana temperature; nieprawidlowa wartosc -> domyslna
    preferences.begin("akwarium", false);
    float stored = preferences.getFloat("targetTemp", defaultTarget);
    targetTemp = isTargetValid(stored) ? stored : defaultTarget;
}

void TemperatureManager::update() {
    const uint32_t now = millis();

    // zlecenie pomiaru (nie czeka na wynik)
    if (!conversionPending) {
        if (now - lastRequestMs >= READ_INTERVAL_MS) {
            sensor.requestTemperatures();
            lastRequestMs = now;
            conversionPending = true;
        }
        return;
    }

    //  czekamy na koniec konwersji bez blokowania loop()
    if (now - lastRequestMs < conversionTimeMs) return;
    conversionPending = false;

    //  odczyt i walidacja
    const float t = sensor.getTempCByIndex(0);

    if (t == DEVICE_DISCONNECTED_C || isnan(t)) {
        handleSensorError("Brak odczytu z czujnika temperatury");
        return;
    }
    if (t == 85.0f) {  // wartosc po resecie DS18B20 - nie jest prawdziwym pomiarem
        handleSensorError("Czujnik zwrocil wartosc 85.0 (reset / zakloceania)");
        return;
    }
    if (t < -55.0f || t > 125.0f) {  // poza zakresem pracy DS18B20
        handleSensorError("Odczyt poza zakresem czujnika");
        return;
    }

    sensorOk = true;
    currentTemp = t;

    // Twardy limit bezpieczenstwa - niezalezny od histerezy i temperatury zadanej
    if (currentTemp >= ABSOLUTE_MAX_TEMP) {
        if (heaterState) {
            Serial.printf("OSTRZEZENIE: temperatura %.2f C przekroczyla limit %.1f C. Grzalka WYLACZONA.\n",
                          currentTemp, ABSOLUTE_MAX_TEMP);
        }
        turnHeaterOff();
        return;
    }

    // Algorytm histerezy
    if (!heaterState && currentTemp < (targetTemp - hysteresis)) {
        turnHeaterOn();
    } else if (heaterState && currentTemp > (targetTemp + hysteresis)) {
        turnHeaterOff();
    }
}

bool TemperatureManager::setTargetTemperature(float newTarget) { // zmiana temperatury zadanej z zapisem do NVS (tylko gdy wartosc sie zmienila)
    if (!isTargetValid(newTarget)) {
        Serial.printf("Odrzucono temperature docelowa (dozwolone %.1f-%.1f C)\n",
                      MIN_TARGET_TEMP, MAX_TARGET_TEMP);
        return false;
    }
    if (fabsf(newTarget - targetTemp) < 0.01f) return true;  // bez zbednego zapisu we flash
 
    targetTemp = newTarget; // zapis do NVS tylko gdy wartosc sie zmienila
    preferences.putFloat("targetTemp", targetTemp);
    Serial.printf("Nowa temperatura docelowa: %.2f (zapisano w NVS)\n", targetTemp);
    return true;
}

void TemperatureManager::handleSensorError(const char* reason) {    // w razie awarii czujnika: grzalka OFF, log co 10 s
    turnHeaterOff();  // w razie awarii zawsze wylaczamy grzalke
    currentTemp = DEVICE_DISCONNECTED_C;

    const uint32_t now = millis();
    if (sensorOk || (now - lastErrorLogMs >= ERROR_LOG_INTERVAL_MS)) {  // log przy zmianie i co 10 s
        Serial.printf("BLAD KRYTYCZNY: %s. Grzalka WYLACZONA.\n", reason);
        lastErrorLogMs = now;
    }
    sensorOk = false;
}

void TemperatureManager::writeRelay(bool on) {  // wlacz / wylacz grzalke (sprzetowo)
    digitalWrite(relayPin, (on == relayActiveHigh) ? HIGH : LOW);
}

void TemperatureManager::turnHeaterOn() { // wlacz grzalke (sprzetowo)
    writeRelay(true);
    if (!heaterState) Serial.println("Grzalka WLACZONA");
    heaterState = true;
}

void TemperatureManager::turnHeaterOff() { // wylacz grzalke (sprzetowo)
    writeRelay(false);  // zawsze zapisujemy pin - stan sprzetu zgodny z oczekiwanym
    if (heaterState) Serial.println("Grzalka WYLACZONA");
    heaterState = false;
}

float TemperatureManager::getCurrentTemp() const { return currentTemp; }
float TemperatureManager::getTargetTemp()  const { return targetTemp; }
bool  TemperatureManager::getHeaterState() const { return heaterState; }
bool  TemperatureManager::isSensorOk()     const { return sensorOk; }
