#ifndef GENERATOR_SERVICE_HPP
#define GENERATOR_SERVICE_HPP

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

struct JobStatus {
    uint64_t id;
    std::string status;
};

struct JobConfig {
    int fileCount = 1;
    int linesPerFile = 1000;
    std::string outputDir = "tmp/";
};

class IGeneratorService {
public:
    virtual ~IGeneratorService() = default;

public:
    virtual JobStatus StartJob(const JobConfig& config) = 0;
    virtual std::optional<JobStatus> GetStatus(uint64_t jobId) = 0;
};

class GeneratorService : public IGeneratorService {
public:
    GeneratorService();

public:
    JobStatus StartJob(const JobConfig& config) override;
    std::optional<JobStatus> GetStatus(uint64_t jobId) override;

private:
    JobStatus RegisterNewJob();
private:
    std::unordered_map<uint64_t, JobStatus> m_allJobsStatus;
    std::atomic<uint64_t> m_nextJobId{0};
};

#endif // GENERATOR_SERVICE_HPP