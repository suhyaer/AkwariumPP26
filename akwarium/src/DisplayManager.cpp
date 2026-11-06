#include "DisplayManager.h"
#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include "AlarmManager.h"
#include "config.h"

DisplayManager::DisplayManager(uint8_t i2cAddr, uint32_t pageMs) // inicjalizacja obiektu
    : display(128, 64, &Wire, -1), addr(i2cAddr), pageMs(pageMs),
      available(false), page(0), pageTotal(BASE_PAGES), pageStartMs(0), lastDrawMs(0) {}

bool DisplayManager::begin() { // wlaczenie wyswietlacza, zwraca false, gdy nie wykryto urzadzenia (ale program dziala dalej)
    available = display.begin(SSD1306_SWITCHCAPVCC, addr);
    if (!available) {
        Serial.println("BLAD: Nie wykryto wyswietlacza OLED - praca bez wyswietlacza");
        return false;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Akwarium");
    display.println("Uruchamianie...");
    display.display();
    pageStartMs = millis();
    lastDrawMs  = millis();
    return true;
}

void DisplayManager::update(const DisplayData& d) {
    if (!available) return;
    const uint32_t now = millis();

    bool redraw = false;
    pageTotal = pageCount(d);
    if (page >= pageTotal) {                     // strona alarmow zniknie, gdy alarmy sie skoncza
        page = 0;
        pageStartMs = now;
        redraw = true;
    }
    if (now - pageStartMs >= pageMs) {           // zmiana strony - odmierzana przez millis()
        page = (page + 1) % pageTotal;
        pageStartMs = now;
        redraw = true;
    }
    if (now - lastDrawMs >= REDRAW_MS) redraw = true;
    if (!redraw) return;

    lastDrawMs = now;
    drawPage(d);
}

// grafika: ramka, wartosc, stopka

void DisplayManager::drawFrame(const char* title, const DisplayData& d) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(title);

    char idx[8];
    snprintf(idx, sizeof(idx), "%u/%u", (unsigned)(page + 1), (unsigned)pageTotal);
    display.setCursor(128 - 6 * (int)strlen(idx), 0);   // numer strony przy prawej krawedzi
    display.print(idx);

    if (d.alarmMask) {                                  // znak "!" w odwroconych kolorach = aktywny alarm
        display.fillRect(92, 0, 8, 9, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(94, 1);
        display.print("!");
        display.setTextColor(SSD1306_WHITE);
    }

    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
}

void DisplayManager::drawValue(const char* value, const char* unit) {
    display.setTextSize(3);                      // znak = 18 x 24 px
    display.setCursor(0, 20);
    display.print(value);
    display.setTextSize(2);                      // mniejszy rozmiar czcionki
    display.setCursor(18 * (int)strlen(value) + 4, 28);
    display.print(unit);
    display.setTextSize(1);
}

void DisplayManager::drawFooter(const char* text) { // stopka na dole ekranu
    display.setTextSize(1);
    display.setCursor(0, 54);
    display.print(text);
}

void DisplayManager::drawAlarmPage(const DisplayData& d) { //strona alarmow
    drawFrame("ALARMY", d);
    display.setTextSize(1);

    uint8_t total = 0;
    for (uint8_t i = 0; i < ALARM_COUNT; i++) if ((d.alarmMask >> i) & 1u) total++;

    const uint8_t maxRows = 5;
    int y = 14;
    uint8_t shown = 0;
    for (uint8_t i = 0; i < ALARM_COUNT; i++) {
        if (!((d.alarmMask >> i) & 1u)) continue;
        if (total > maxRows && shown == maxRows - 1) break;   // ostatni wiersz zostaje na "+N wiecej"
        display.setCursor(0, y);
        display.print(AlarmManager::name((AlarmId)i));
        y += 9;
        shown++;
    }
    if (total > shown) {
        char more[24];
        snprintf(more, sizeof(more), "+%u wiecej", (unsigned)(total - shown));
        display.setCursor(0, y);
        display.print(more);
    }
}

void DisplayManager::drawPage(const DisplayData& d) { // rysuje strone w zaleznosci od numeru 'page'
    char val[16];
    char foot[28];

    switch (page) {
    case 0:   // temperatura
        drawFrame("TEMPERATURA", d);
        if (d.tempOk) {
            snprintf(val, sizeof(val), "%.1f", d.tempC);
            drawValue(val, "C");
            snprintf(foot, sizeof(foot), "Zad %.1fC Grzalka:%s", d.targetC, d.heaterOn ? "ON" : "OFF");
        } else {
            drawValue("BLAD", "");
            snprintf(foot, sizeof(foot), "Brak czujnika! Grz.OFF");
        }
        drawFooter(foot);
        break;

    case 1:   // TDS
        drawFrame("TDS", d);
        snprintf(val, sizeof(val), "%.0f", d.tdsPpm);
        drawValue(val, "ppm");
        snprintf(foot, sizeof(foot), "czujnik %.0f mV", d.tdsMv);
        drawFooter(foot);
        break;

    case 2:   // zmetnienie pokazuje przejrzystosc wzgledem czystej wody
        drawFrame("PRZEJRZYSTOSC", d);
        snprintf(val, sizeof(val), "%.0f", d.clarityPct);
        drawValue(val, "%");
        snprintf(foot, sizeof(foot), "czujnik %.2f V", d.turbidityV);
        drawFooter(foot);
        break;

    case 3:   // swiatlo
        drawFrame("SWIATLO", d);
        if (d.lightOk) {
            snprintf(val, sizeof(val), "%.0f", d.lux);
            drawValue(val, "lx");
            drawFooter("natezenie swiatla");
        } else {
            drawValue("BLAD", "");
            drawFooter("Brak czujnika BH1750");
        }
        break;

    case 4:   // napowietrzanie
        drawFrame("NAPOWIETRZANIE", d);
        if (d.pressureOk) {
            snprintf(val, sizeof(val), "%.1f", d.pressureKpa);
            drawValue(val, "kPa");
            drawFooter("cisnienie w ukladzie");
        } else {
            drawValue("BLAD", "");
            drawFooter("Brak odczytu cisnienia");
        }
        break;

    case 5:   // Wi-Fi: adres IP potrzebny do otwarcia strony w przegladarce
        drawFrame("WI-FI", d);
        display.setTextSize(1);
        if (d.wifiConnected && d.ip[0]) {
            display.setCursor(0, 16); display.print("POLACZONO");
            snprintf(foot, sizeof(foot), "IP: %s", d.ip);
            display.setCursor(0, 30); display.print(foot);
            snprintf(foot, sizeof(foot), "http://%s.local", MDNS_HOSTNAME);
            display.setCursor(0, 42); display.print(foot);
            snprintf(foot, sizeof(foot), "sygnal %d dBm", d.wifiRssi);
            drawFooter(foot);
        } else {
            display.setCursor(0, 16); display.print("BRAK POLACZENIA");
            display.setCursor(0, 30); display.print("laczenie z siecia...");
            drawFooter("siec 2,4 GHz? haslo?");
        }
        break;

    default:  // 6: alarmy (tylko gdy sa aktywne)
        drawAlarmPage(d);
        break;
    }
    display.display();   // wysylka bufora do ekranu po I2C
}
