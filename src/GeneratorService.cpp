#include "GeneratorService.hpp"

GeneratorService::GeneratorService() {}

JobStatus GeneratorService::StartJob(const JobConfig& config) {
    JobStatus job_status = RegisterNewJob();
    return {0, ""}; // temp
}

std::optional<JobStatus> GeneratorService::GetStatus(uint64_t job_id) {
    auto it = m_all_jobs_status.find(job_id);

    if (it != m_all_jobs_status.end())
        return it->second;

    return std::nullopt;
}

JobStatus GeneratorService::RegisterNewJob() {
    JobStatus new_job;
    new_job.id = ++m_next_job_id;
    new_job.status = "running";

    m_all_jobs_status[new_job.id] = new_job;
    return new_job;
}