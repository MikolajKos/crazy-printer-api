#ifndef GENERATOR_SERVICE_HPP
#define GENERATOR_SERVICE_HPP

#include "LogGenerator.hpp"
#include "ThreadPool.hpp"
#include "ThreadSafeQueue.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

struct JobStatus {
    uint64_t id;
    std::optional<double> execution_time_seconds;
    std::string status;
    uint32_t files_written;
};

struct JobConfig {
    std::string output_dir = "tmp/";
    uint32_t file_count = 10;
    uint32_t lines_per_file = 10000;
    uint32_t producer_threads = 4;
    uint32_t consumer_threads = 4;
};

struct Batch {
    std::string data;
    uint32_t line_count;
    size_t size() const { return data.size(); }
};

struct JobContext {
    ThreadSafeQueue<Batch> queue{256 * 1024 * 1024};
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    double execution_time_seconds = 0.0;
    uint64_t id;
    
    std::unique_ptr<ThreadPool> producers;
    std::unique_ptr<ThreadPool> consumers;
    
    std::string status;

    std::atomic<uint32_t> active_producers{0};

    std::atomic<uint32_t> next_file_id{0};
    std::atomic<uint32_t> files_fully_written{0};
    std::atomic<uint32_t> lines_produced{0};

    explicit JobContext(uint64_t job_id)
        :id(job_id), 
        start_time(std::chrono::steady_clock::now()) {}

    void MarkAsFinished() {
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;
        execution_time_seconds = elapsed.count();

        status = "done";
    }
};

class IGeneratorService {
public:
    virtual ~IGeneratorService() = default;

public:
    virtual JobStatus StartJob(const JobConfig& config) = 0;
    virtual std::optional<JobStatus> GetStatus(uint64_t job_id) = 0;
};

class GeneratorService : public IGeneratorService {
public:
    GeneratorService();

public:
    JobStatus StartJob(const JobConfig& config) override;
    std::optional<JobStatus> GetStatus(uint64_t job_id) override;

private:
    JobStatus RegisterNewJob();
    void ProducerTask(std::shared_ptr<JobContext> context, JobConfig config);
    void ConsumerTask(std::shared_ptr<JobContext> context, JobConfig config);
private:
    std::unordered_map<uint64_t, std::shared_ptr<JobContext>> m_active_jobs;
    
    std::atomic<uint64_t> m_next_job_id{0};
    std::mutex m_mutex;
};

#endif // GENERATOR_SERVICE_HPP