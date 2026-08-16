#include "GeneratorService.hpp"

GeneratorService::GeneratorService() {}

JobStatus GeneratorService::StartJob(const JobConfig& config) {
    JobStatus jobStatus = RegisterNewJob();
    
}

std::optional<JobStatus> GeneratorService::GetStatus(uint64_t jobId) {
    auto it = m_allJobsStatus.find(jobId);

    if (it != m_allJobsStatus.end())
        return it->second;

    return std::nullopt;
}

JobStatus GeneratorService::RegisterNewJob() {
    JobStatus newJob;
    newJob.id = ++m_nextJobId;
    newJob.status = "running";

    m_allJobsStatus[newJob.id] = newJob;
    return newJob;
}