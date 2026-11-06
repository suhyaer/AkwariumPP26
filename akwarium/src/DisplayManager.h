#pragma once
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SystemStatus.h"
using DisplayData = SystemStatus;   // OLED wykorzystuje te same dane co serwer WWW

// Wyswietlacz OLED 128x64 (SSD1306). Pokazuje po jednym parametrze, zmieniajac je co 'pageMs'.
// Gdy sa aktywne alarmy pokazuje sie strona "ALARMY" w rotacji i ostrzezenie "!" na kazdej stronie.
// Nie ma polskich znakow
class DisplayManager {
public:
    DisplayManager(uint8_t i2cAddr, uint32_t pageMs);
    bool begin();                           // false = brak wyswietlacza urzadzenie dziala bez niego
    void update(const DisplayData& d);      // wywolanie petli

private:
    static constexpr uint8_t  BASE_PAGES = 6;   // ilosc stron
    static constexpr uint32_t REDRAW_MS  = 500;   // Czestotliwosc odswiezania ekranu (bez zmiany strony) 

    Adafruit_SSD1306 display;
    uint8_t  addr;
    uint32_t pageMs;
    bool     available;
    uint8_t  page;
    uint8_t  pageTotal;
    uint32_t pageStartMs;
    uint32_t lastDrawMs;

    uint8_t pageCount(const DisplayData& d) const { return BASE_PAGES + (d.alarmMask ? 1 : 0); }
   // rysowanie poszczegolnych stron
    void drawPage(const DisplayData& d);
    void drawAlarmPage(const DisplayData& d);
    void drawFrame(const char* title, const DisplayData& d);
    void drawValue(const char* value, const char* unit);
    void drawFooter(const char* text);
};
