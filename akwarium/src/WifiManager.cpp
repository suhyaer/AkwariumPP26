#include "WifiManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"

WifiManager::WifiManager(const char* ssid, const char* password, int channel)
    : ssid(ssid), password(password), channel(channel), connected(false), lastAttemptMs(0) { ipStr[0] = '\0'; }

void WifiManager::begin() {          // inicjalizacja Wi-Fi, asynchroniczne laczenie
    WiFi.persistent(false);          // nie zapisuj danych sieci do flash przy kazdym laczeniu
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password, channel);   // asynchronicznie - wraca od razu
    lastAttemptMs = millis();
    Serial.printf("Wi-Fi: laczenie z siecia \"%s\"...\n", ssid);
}

void WifiManager::update() {                // wywoalanie w loop(); pilnuje ponownego laczenia
    const bool now = (WiFi.status() == WL_CONNECTED);
    const uint32_t t = millis();

    if (now && !connected) {        // polaczenie zostalo nawiazane
        snprintf(ipStr, sizeof(ipStr), "%s", WiFi.localIP().toString().c_str());
        Serial.printf("Wi-Fi: polaczono, adres IP: %s\n", ipStr);
        MDNS.end();
        if (MDNS.begin(MDNS_HOSTNAME)) {        // mDNS dostepne, rejestracja hosta i uslugi HTTP
            MDNS.addService("http", "tcp", 80);
            Serial.printf("Strona urzadzenia: http://%s  lub  http://%s.local\n", ipStr, MDNS_HOSTNAME);
        } else {        // mDNS niedostepne (np. Android) - tylko adres IP
            Serial.printf("Strona urzadzenia: http://%s  (mDNS niedostepny)\n", ipStr);
        }
    } else if (!now && connected) {             // polaczenie zostalo utracone
        Serial.println("Wi-Fi: utracono polaczenie (sterowanie grzalka dziala dalej)");
        ipStr[0] = '\0';
        lastAttemptMs = t;
    }
    connected = now;

    if (!connected && (t - lastAttemptMs >= RECONNECT_AFTER_MS)) {      // ponowne laczenie co 15 s
        Serial.println("Wi-Fi: ponawiam laczenie...");
        WiFi.disconnect();
        WiFi.begin(ssid, password, channel);
        lastAttemptMs = t;
    }
}

int WifiManager::rssi() const { return connected ? WiFi.RSSI() : 0; }
