#pragma once
#include <stdint.h>
#include <WebServer.h>
#include "SystemStatus.h"
#include "TemperatureManager.h"

// Serwer HTTP:  GET /            - strona z podgladem i formularzem
//               GET /api/status  - stan urzadzenia w JSON
//               POST /api/target?value=26.5 - zmiana temperatury zadanej
// Synchroniczny WebServer: obsluga zadan odbywa sie w petli tej samej co sterowanie
class WebInterface {                    
public:
    WebInterface(TemperatureManager& tempMgr, uint16_t port = 80);
    void begin();
    void update(const SystemStatus& status);    // wywolanie w loop()

private:
    WebServer          server;
    TemperatureManager& tempMgr;
    SystemStatus       status;

    void handleRoot();
    void handleStatus();
    void handleTarget();
    void handleNotFound();
    void sendJsonError(int code, const char* message);
};
