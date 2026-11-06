#pragma once
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#if __has_include(<esp_arduino_version.h>)
  #include <esp_arduino_version.h>
#endif

// Watchdog: jesli loop() nie wywoła watchdogFeed() w ciagu 'timeoutS', ESP32 sie restartuje.
// Po restarcie begin() ustawia grzalke w stan OFF, wiec zawieszenie nie grzeje w nieskonczonosc.
inline void watchdogBegin(uint32_t timeoutS) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    esp_task_wdt_config_t cfg;                 // starsze wydanie ma inny interface
    cfg.timeout_ms     = timeoutS * 1000;
    cfg.idle_core_mask = 0;
    cfg.trigger_panic  = true;
    esp_task_wdt_reconfigure(&cfg);
#else
    esp_task_wdt_init(timeoutS, true);         
#endif
    esp_task_wdt_add(NULL);                    // obserwuj biezace zadanie (loop)
}

inline void watchdogFeed() { esp_task_wdt_reset(); }

inline const char* resetReasonText() { // zwraca tekstowa reprezentacje powodu ostatniego restartu
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "wlaczenie zasilania";
        case ESP_RST_EXT:       return "reset zewnetrzny (przycisk EN)";
        case ESP_RST_SW:        return "restart programowy";
        case ESP_RST_PANIC:     return "wyjatek / panic";
        case ESP_RST_INT_WDT:   return "watchdog (przerwania)";
        case ESP_RST_TASK_WDT:  return "watchdog zadania (zawieszenie petli)";
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_BROWNOUT:  return "spadek napiecia (brownout!)";
        case ESP_RST_DEEPSLEEP: return "wybudzenie z deep sleep";
        default:                return "nieznany";
    }
}
