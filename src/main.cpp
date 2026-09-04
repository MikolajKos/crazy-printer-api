#include "GeneratorService.hpp"
#include "LoggerSetup.hpp"
#include "PrinterController.hpp"

#include "httplib.h"

#include <spdlog/spdlog.h>

int main() {
    LoggerSetup::Init();
    LoggerSetup::PrintWelcomeMessage();

    spdlog::info("Waking up the printer...");

    httplib::Server svr;
    GeneratorService service;
    PrinterController controller(service);

    controller.RegisterRoutes(svr);

    std::string host = "0.0.0.0";
    int port = 8080;

    spdlog::info("Listening on host: {} port: {}", host, port);

    svr.listen(host, port);

    return 0;
}