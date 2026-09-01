#include "GeneratorService.hpp"

#include <spdlog/spdlog.h>

GeneratorService::GeneratorService() {}

JobStatus GeneratorService::StartJob(const JobConfig& config) {
    JobStatus job_status = RegisterNewJob();

    
    // Prepare output catalog
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

    context->active_producers.fetch_sub(1);
}

void GeneratorService::ConsumerTask(std::shared_ptr<JobContext> context, JobConfig config) {    
    // Pass remaining lines to next file in case batch.line_count > lines_needed
    std::string leftover_data;
    uint32_t leftover_lines = 0;
    
    while (true) {
        const uint32_t current_file_id = context->next_file_id.fetch_add(1);
        
        if (current_file_id >= config.file_count) {
            // At the end of thread lifetime return leftovers to the queue
            if (leftover_lines > 0)
                context->queue.push(Batch{std::move(leftover_data), leftover_lines});
            
            break;
        }

        std::string filename = CreateFilename(config.output_dir, current_file_id, context->id);
        std::optional<std::ofstream> opt_file = OpenLogFile(filename);

        if (!opt_file) {
            // In case of error treat current file as fully written to avoid incomplete task
            context->files_fully_written.fetch_add(1);
            continue;
        }

        std::ofstream file = std::move(*opt_file);
        bool queue_exhausted = false;
        uint32_t lines_needed = config.lines_per_file;

        auto process_chunk = [&](std::string_view data, uint32_t lines_in_data) {
            if (lines_needed >= lines_in_data) {
                file.write(data.data(), data.size());                
                lines_needed -= lines_in_data;
                leftover_data.clear();

                leftover_lines = 0;
            } else { // Edge case: leftover_lines > lines_per_file; Pass lines to another iteration
                size_t split_pos = FindNthNewLine(data, lines_needed);
                file.write(data.data(), split_pos);
                leftover_data = std::string(data.substr(split_pos));
                leftover_lines = lines_in_data - lines_needed;
                
                lines_needed = 0;
            }
        };
        
        // First write remaining lines
        if (leftover_lines > 0) {
            std::string temp_leftovers = std::move(leftover_data);
            process_chunk(temp_leftovers, leftover_lines);
        }

        while (lines_needed > 0) {
            // Take single batch off the queue
            std::optional<Batch> opt_batch = context->queue.pop();

            if (!opt_batch) {
                queue_exhausted = true;
                break;
            }

            Batch batch = std::move(*opt_batch);
            process_chunk(batch.data, batch.line_count);
        }

        file.close();

        if (context->files_fully_written.fetch_add(1) == config.file_count - 1)
            JobTeardown(context);
        
        if (queue_exhausted) break;
    }
}

size_t GeneratorService::FindNthNewLine(std::string_view data, size_t n) {
    size_t pos = 0;
    for (size_t i = 0; i < n; ++i) {
        pos = data.find('\n', pos);
        if (pos == std::string_view::npos) return data.size(); // n less than whole file line count
        pos++; // next line
    }
    return pos;
}

std::string GeneratorService::CreateFilename(std::string_view dir, const uint32_t file_id, const uint32_t job_id) {
    return std::format(
            "{}/logs_{}_job_id_{}.log",
            dir,
            file_id,
            job_id
    );
}

std::optional<std::ofstream> GeneratorService::OpenLogFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    
    if (!file.is_open())
        return std::nullopt;

    return file;
}

void GeneratorService::JobTeardown(std::shared_ptr<JobContext> context) {
    {        
        std::lock_guard<std::mutex> lock(m_mutex);
        context->MarkAsFinished();
    }

    context->queue.markDone();

    spdlog::info("Job {} fully completed in {:.3f} s.", 
        context->id, 
        context->execution_time_seconds);
}