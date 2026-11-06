#pragma once
#include <stdint.h>

// Lacznosc Wi-Fi w trybie stacji. Laczenie dzieje sie w tle,
// a sterowanie grzalka dziala niezaleznie od tego, czy siec jest dostepna.
class WifiManager {
public:
    WifiManager(const char* ssid, const char* password, int channel = 0);
    void begin();
    void update();                       // wywolywane w loop(); pilnuje ponownego laczenia

    bool isConnected() const { return connected; }
    int  rssi() const;
    const char* ip() const { return ipStr; }   // "" gdy brak polaczenia

private:
    static constexpr uint32_t RECONNECT_AFTER_MS = 15000;
    const char* ssid;
    const char* password;
    int      channel;
    bool     connected;
    uint32_t lastAttemptMs;
    char     ipStr[16];
};
