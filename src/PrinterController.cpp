#include "httplib.h"
#include "PrinterController.hpp"

#include <chrono>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string>

PrinterController::PrinterController(IGeneratorService& service): m_service(service) {}

void PrinterController::RegisterRoutes(httplib::Server& svr) {
    svr.Post("/api/printer/start", [this](const httplib::Request& req, httplib::Response& res) {
        auto start_time = std::chrono::steady_clock::now();
        spdlog::info("Incoming POST /api/printer/start - Payload: {}", req.body);

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
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            spdlog::error("Failed POST /api/printer/start - 500 Bad Request - {} ms - Exception: {}", duration, e.what());
            
            return;
        }
        
        const JobStatus status = m_service.StartJob(config);

        nlohmann::json response = { { "jobId", status.id}, {"status", status.status} };
        res.set_content(response.dump(), "application/json");
        res.status = 202; // Accepted

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        ).count();
        spdlog::info("Accepted POST /api/printer/start - {} Accepted - {} ms", res.status, duration);
    });

    svr.Get("/api/printer/status/:id", [this](const httplib::Request& req, httplib::Response& res) {
        auto start_time = std::chrono::steady_clock::now();
        
        uint64_t job_id;
        
        try {
            job_id = std::stoull(req.path_params.at("id"));
            spdlog::info("Incoming GET /api/printer/status/{} - Path Parameter: {}", job_id, job_id);
        }
        catch (const std::exception& e) {
            res.set_content("Bad Request", "text/plain");
            res.status = 400; // Bad Request

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            spdlog::error("Failed GET /api/printer/status/{} - {} Bad Request - {} ms - Exception: {}", job_id, res.status, duration, e.what());
            
            return;
        }

        const auto status = m_service.GetStatus(job_id);

        if (!status.has_value()) {
            res.set_content("Not Found", "text/plain");
            res.status = 404; // Not Found
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            spdlog::warn("Not Found GET /api/printer/status/{} - {} Not Found - {} ms", job_id, res.status, duration);

            return;            
        }
        
        const auto& job_status = status.value();

        nlohmann::json response = {
            {"jobId", job_status.id}, 
            {"status", job_status.status},
            {"filesWritten", job_status.files_written}
        };

        if (job_status.status == "done") {
            response["metrics"] = {
                {"executionTimeSeconds", job_status.execution_time_seconds.value()}
            };
        }

        res.set_content(response.dump(), "application/json");
        res.status = 200; // OK

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        ).count();
        spdlog::info("Completed GET /api/printer/status/{} - {} OK - {} ms", job_id, res.status, duration);
    });
}