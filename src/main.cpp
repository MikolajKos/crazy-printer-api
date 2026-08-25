#include "httplib.h"
#include "PrinterController.hpp"

int main() {
    httplib::Server server;

    GeneratorService service;
    PrinterController controller(service);

    controller.RegisterRoutes(server);

    return 0;
}