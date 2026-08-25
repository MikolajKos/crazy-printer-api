#include "GeneratorService.hpp"

#include <spdlog/spdlog.h>

GeneratorService::GeneratorService() {}

JobStatus GeneratorService::StartJob(const JobConfig& config) {
    JobStatus job_status = RegisterNewJob();

    
    // Prepare output catalog
    std::filesystem::remove_all(config.output_dir);
    std::filesystem::create_directories(config.output_dir);
    
    // Fetch context
    std::shared_ptr<JobContext> context;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        context = m_active_jobs[job_status.id];
    }

    // Run producers and consumers
    context->active_producers = config.producer_threads;
    
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
    
    // Active/Completed Jobs
    const auto it = m_active_jobs.find(job_id);

    if (it == m_active_jobs.end()) {
        return std::nullopt;   
    }
    
    const auto context = it->second;

    JobStatus job_status;

    if (context->status == "done") {
        job_status.execution_time_seconds = context->execution_time_seconds;
    }

    job_status.id = context->id;
    job_status.status = context->status;
    job_status.files_written = context->files_fully_written;

    return job_status;
}

JobStatus GeneratorService::RegisterNewJob() {
    auto context = std::make_shared<JobContext>(++m_next_job_id);
    context->status = "running";

    spdlog::info("New job received (id: {})", context->id);
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_active_jobs[context->id] = context;
    }
    
    JobStatus job_status;
    job_status.id = context->id;
    job_status.status = context->status;
    job_status.files_written = 0;

    return job_status;
}

void GeneratorService::ProducerTask(std::shared_ptr<JobContext> context, JobConfig config) {
    const uint32_t total_lines_needed = config.file_count * config.lines_per_file;
    
    // Batch size capped at lines_per_file to prevent a single batch from overflowing one file
    const uint32_t batch_size = std::min(100000u, config.lines_per_file);

    while (true) {
        const uint32_t current_size = context->lines_produced.fetch_add(batch_size);

        if (current_size >= total_lines_needed)
            break;

        const uint32_t to_produce = std::min(batch_size, total_lines_needed - current_size);

        std::string logs;
        logs.reserve(to_produce * 100); // Assuming that each line has about 100 signs

        // Get one timestamp for each batch to avoid calling chono in a loop
        const auto timestamp = LogGenerator::GetCurrentTimestamp();
        
        for (uint32_t i = 0; i < to_produce; ++i) {
            LogGenerator::GenerateLine(timestamp, logs);
        }

        context->queue.push(Batch{ std::move(logs), to_produce });
    }

    // Job done - last producer finished its job, raise 'done' flag, wake up consumers
    if (context->active_producers.fetch_sub(1) == 1)
        context->queue.markDone();
}

void GeneratorService::ConsumerTask(std::shared_ptr<JobContext> context, JobConfig config) {    
    while (true) {
        uint32_t lines_written = 0;
        const uint32_t current_file_id = context->next_file_id.fetch_add(1);
        
        if (current_file_id >= config.file_count)
            break;

        std::string filename = std::format(
            "{}/logs_{}_job_id_{}.log",
            config.output_dir,
            current_file_id,
            context->id
        );

        std::ofstream file(filename, std::ios::out | std::ios::binary);

        if (!file.is_open())
            continue;
        
        bool queue_exhausted = false;
        
        while (lines_written < config.lines_per_file) {
            // Take single batch off the queue
            std::optional<Batch> batch = context->queue.pop();

            if (!batch.has_value()) {
                queue_exhausted = true;
                break;
            }

            file.write(batch.value().data.data(), batch.value().size());
            lines_written += batch.value().line_count;
        }

        file.close();

        if (context->files_fully_written.fetch_add(1) == config.file_count - 1) {
            std::lock_guard<std::mutex> lock(m_mutex);
            context->MarkAsFinished();

            spdlog::info("Job {} fully completed in {:.3f} s.", 
                context->id, 
                context->execution_time_seconds);
        }
        
        if (queue_exhausted) break;
    }
}