#pragma once
// Parser komend firmware diagnostycznego testowany samotestem
#include <string.h>
#include <ctype.h>
#include "WebApi.h"

enum class BringupCmd {  // typ komend firmware diagnostycznego
    NONE, HELP, SCAN, STREAM_ON, STREAM_OFF, RELAY_ON, RELAY_OFF,
    CAL_ZERO, CAL_PRESS, CAL_TDS, CAL_CLEAR, BAD_ARG, UNKNOWN
};

struct BringupParsed { 
    BringupCmd cmd = BringupCmd::NONE;
    float arg = 0.0f;
};

inline BringupParsed parseBringupCommand(const char* line) { // parsuje linie z Serial i zwraca typ komendy i argument
    BringupParsed r;
    if (!line) return r;

    char buf[48];
    size_t n = 0;
    for (; line[n] && n < sizeof(buf) - 1; n++) buf[n] = (char)tolower((unsigned char)line[n]);
    buf[n] = '\0';

    char* save = nullptr;
    char* t0 = strtok_r(buf, " \t", &save);
    if (!t0) return r;                                   // przerwa
    char* t1 = strtok_r(nullptr, " \t", &save);
    char* t2 = strtok_r(nullptr, " \t", &save);

    auto is = [](const char* a, const char* b) { return a && strcmp(a, b) == 0; };

    if (is(t0, "help") || is(t0, "?"))  { r.cmd = BringupCmd::HELP; return r; } //odpowiedz konsoli
    if (is(t0, "scan"))                 { r.cmd = BringupCmd::SCAN; return r; }
    if (is(t0, "stream")) {
        if (is(t1, "on"))  { r.cmd = BringupCmd::STREAM_ON;  return r; }
        if (is(t1, "off")) { r.cmd = BringupCmd::STREAM_OFF; return r; }
        r.cmd = BringupCmd::BAD_ARG; return r;
    }
    if (is(t0, "relay")) {
        if (is(t1, "on"))  { r.cmd = BringupCmd::RELAY_ON;  return r; }
        if (is(t1, "off")) { r.cmd = BringupCmd::RELAY_OFF; return r; }
        r.cmd = BringupCmd::BAD_ARG; return r;
    }
    if (is(t0, "cal")) {
        if (is(t1, "zero"))  { r.cmd = BringupCmd::CAL_ZERO;  return r; }
        if (is(t1, "clear")) { r.cmd = BringupCmd::CAL_CLEAR; return r; }
        if (is(t1, "press") || is(t1, "tds")) {
            float v;
            if (!t2 || !WebApi::parseTargetParam(t2, v) || v <= 0.0f) { r.cmd = BringupCmd::BAD_ARG; return r; }
            r.cmd = is(t1, "press") ? BringupCmd::CAL_PRESS : BringupCmd::CAL_TDS;
            r.arg = v;
            return r;
        }
        r.cmd = BringupCmd::BAD_ARG; return r;
    }
    r.cmd = BringupCmd::UNKNOWN;
    return r;
}
