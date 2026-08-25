#include "GeneratorService.hpp"
#include "LoggerSetup.hpp"
#include "PrinterController.hpp"

#include "httplib.h"

int main() {
    LoggerSetup::Init();

    httplib::Server svr;
    GeneratorService service;
    PrinterController controller(service);

    controller.RegisterRoutes(svr);

    svr.listen("0.0.0.0", 8080);
    
    return 0;
}