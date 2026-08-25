#include "httplib.h"
#include "PrinterController.hpp"

#include <cstdint>
#include <string>

PrinterController::PrinterController(IGeneratorService& service): m_service(service) {}

void PrinterController::RegisterRoutes(httplib::Server& svr) {
    svr.Post("/api/printer/start", [this](const httplib::Request& req, httplib::Response& res) {
        JobConfig config;
        
        try {
            const auto body = nlohmann::json::parse(req.body);
            
            // Prepare job config
            config.output_dir = body.at("outputDir").get<std::string>();
            config.file_count = body.at("fileCount").get<uint32_t>();
            config.lines_per_file = body.at("linesPerFile").get<uint32_t>();
            config.producer_threads = body.at("producerThreads").get<uint32_t>();
            config.consumer_threads = body.at("consumerThreads").get<uint32_t>();   
        }
        catch (const nlohmann::json::exception& e) {
            res.set_content("Bad Request", "text/plain");
            res.status = 400; // Bad Request
            return;
        }
        
        const JobStatus status = m_service.StartJob(config);

        nlohmann::json response = { { "jobId", status.id}, {"status", status.status} };
        res.set_content(response.dump(), "application/json");
        res.status = 202; // Accepted
    });

    svr.Get("/api/printer/status/:id", [this](const httplib::Request& req, httplib::Response& res) {
        uint64_t job_id;

        try {
            job_id = std::stoull(req.path_params.at("id"));
        }
        catch (const std::exception& e) {
            res.set_content("Bad Request", "text/plain");
            res.status = 400; // Bad Request
            return;
        }
        
        auto status = m_service.GetStatus(job_id);

        if (!status.has_value()) {
            res.set_content("Not Found", "text/plain");
            res.status = 404; // Not Found
            return;            
        }
        
        nlohmann::json response = {{"jobId", status.value().id}, {"status", status.value().status}};
        res.set_content(response.dump(), "application/json");
        res.status = 200; // OK
    });
}