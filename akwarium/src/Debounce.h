#pragma once
#include <stdint.h>

// Przeciwdziala "migotaniu" alarmow: warunek musi trwac nieprzerwanie 'onMs', zeby wlaczyc stan,
// i znikac nieprzerwanie 'offMs', zeby go wylaczyc.
struct Debounce {
    bool     active   = false;
    bool     lastCond = false;
    uint32_t since    = 0;

    bool update(bool cond, uint32_t now, uint32_t onMs, uint32_t offMs) {
        if (cond != lastCond) { lastCond = cond; since = now; }   // zmiana warunku -> odliczanie od nowa
        const uint32_t held = now - since;                        // arytmetyka bez znaku: bezpieczna przy przepelnieniu millis()
        if (cond && !active && held >= onMs)  active = true;
        if (!cond && active && held >= offMs) active = false;
        return active;
    }
};
