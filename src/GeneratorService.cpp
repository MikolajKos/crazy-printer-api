#include "GeneratorService.hpp"

GeneratorService::GeneratorService() {}

JobStatus GeneratorService::StartJob(const JobConfig& config) {
    JobStatus job_status = RegisterNewJob();

    std::shared_ptr<JobContext> context;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        context = m_active_jobs[job_status.id];
    }

    context->producers = std::make_unique<ThreadPool>(
        config.producer_threads,
        [this, context, config]() { ProducerTask(context, config); }
    );

    context->consumers = std::make_unique<ThreadPool>(
        config.consumer_threads,
        [this, context, config]() { ConsumerTask(context, config); }
    );
    
    return job_status;
}

std::optional<JobStatus> GeneratorService::GetStatus(uint64_t job_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_active_jobs.find(job_id);

    if (it != m_active_jobs.end()) {
        auto context = it->second;
        return JobStatus{context->id, context->status};
    }

    return std::nullopt;
}

JobStatus GeneratorService::RegisterNewJob() {
    auto context = std::make_shared<JobContext>();
    context->id = ++m_next_job_id;
    context->status = "running";

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_active_jobs[context->id] = context;
    }
    
    return JobStatus{context->id, context->status};
}

void GeneratorService::ProducerTask(std::shared_ptr<JobContext> context, JobConfig config) {
    const uint32_t total_lines_needed = config.file_count * config.lines_per_file;
    const uint32_t batch_size = 10000;

    while (true) {
        const uint32_t current_size = context->lines_produced.fetch_add(batch_size);

        if (current_size >= total_lines_needed)
            break;

        const uint32_t to_produce = std::min(batch_size, total_lines_needed - current_size);

        std::string batch;
        batch.reserve(to_produce * 100); // Assuming that each line has about 100 signs

        auto timestamp = LogGenerator::GetCurrentTimestamp();
        auto last_update = std::chrono::system_clock::now();

        for (uint32_t i = 0; i < to_produce; ++i) {
            // Update timestamp every 100ms
            auto now = std::chrono::system_clock::now();
            if ((now - last_update) >= std::chrono::milliseconds(100)) {
                timestamp = LogGenerator::GetCurrentTimestamp();
                last_update = now;
            }

            batch += LogGenerator::GenerateLine(timestamp);
        }

        context->queue.push(std::move(batch));
    }
}

void GeneratorService::ConsumerTask(std::shared_ptr<JobContext> context, JobConfig config) {    
    while (true) {
        uint32_t lines_written = 0;
        const uint32_t current_files_completed = context->files_completed.fetch_add(1);
        
        if (current_files_completed >= config.file_count)
            break;

        std::string filename = std::format(
            "{}/logs_{}_job_id_{}.log",
            config.output_dir,
            current_files_completed,
            context->id
        );

        std::ofstream file;

        if (std::filesystem::exists(filename))
            std::filesystem::remove(filename);

        file.open(filename);

        if (!file.is_open())
            break;
        
        while (lines_written < config.lines_per_file) {
            // Take batch off the queue
            std::optional<std::string> batch = context->queue.pop();

            if (!batch.has_value())
                break;

            file << batch.value();
            lines_written += 
        }

        file.close();
    }
}