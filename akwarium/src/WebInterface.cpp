#include "WebInterface.h"
#include <Arduino.h>
#include "WebApi.h"
#include "WebPage.h"

WebInterface::WebInterface(TemperatureManager& tm, uint16_t port)
    : server(port), tempMgr(tm), status() {}

void WebInterface::begin() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/target", HTTP_POST, [this]() { handleTarget(); });
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
    Serial.println("Serwer WWW uruchomiony (port 80)");
}

void WebInterface::update(const SystemStatus& s) {
    status = s;                 // aktualizacja stanu
    server.handleClient();
}

void WebInterface::handleRoot() {       // wysyla strone HTML z interfejsem WWW
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void WebInterface::handleStatus() {         // wysyla JSON ze stanem urzadzenia
    char buf[768];
    const int n = WebApi::buildStatusJson(status, buf, sizeof(buf));
    if (n <= 0 || (size_t)n >= sizeof(buf)) {
        sendJsonError(500, "blad budowania odpowiedzi");
        return;
    }
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", buf);
}

void WebInterface::handleTarget() {         // ustawia temperature zadana 
    if (!server.hasArg("value")) {
        sendJsonError(400, "brak parametru value");
        return;
    }
    float v;                // nowa temperatura zadana
    if (!WebApi::parseTargetParam(server.arg("value").c_str(), v)) {
        sendJsonError(400, "wartosc nie jest liczba");
        return;
    }
    if (!tempMgr.setTargetTemperature(v)) {       // sprawdza zakres
        char msg[64];
        snprintf(msg, sizeof(msg), "wartosc poza zakresem %.0f-%.0f", TemperatureManager::MIN_TARGET_TEMP,
                 TemperatureManager::MAX_TARGET_TEMP);
        sendJsonError(400, msg);
        return;
    }
    char buf[64];               // wysyla JSON z potwierdzeniem i aktualna temperatura zadana
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"target\":%.2f}", tempMgr.getTargetTemp());
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", buf);
}

void WebInterface::handleNotFound() {               // wysyla 404
    if (server.uri() == "/api/target") {          // istnieje, ale tylko dla POST
        server.sendHeader("Allow", "POST");
        sendJsonError(405, "uzyj metody POST");
        return;
    }
    server.send(404, "text/plain", "404");
}

void WebInterface::sendJsonError(int code, const char* message) {       // wysyla JSON z bledem
    char buf[128];
    snprintf(buf, sizeof(buf), "{\"ok\":false,\"error\":\"%s\"}", message);
    server.send(code, "application/json", buf);
}
