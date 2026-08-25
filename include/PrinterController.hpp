#ifndef PRINTER_CONTROLLER_HPP
#define PRINTER_CONTROLLER_HPP

#include "GeneratorService.hpp"
#include "json.hpp"

namespace httplib { class Server; }

class PrinterController {
public:
    explicit PrinterController(IGeneratorService& service);

    void RegisterRoutes(httplib::Server& svr);
private:
    IGeneratorService& m_service;
};

#endif // PRINTER_CONTROLLER_HPP