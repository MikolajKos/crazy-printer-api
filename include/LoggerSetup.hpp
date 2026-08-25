#ifndef LOGGER_SETUP_HPP
#define LOGGER_SETUP_HPP

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

class LoggerSetup {
public:
    static void Init() {
        spdlog::init_thread_pool(8192, 1);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        auto logger = std::make_shared<spdlog::async_logger>(
            "api_logger", console_sink, spdlog::thread_pool(), spdlog::async_overflow_policy::block
        );
        
        spdlog::set_default_logger(logger);

        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
    }
};

#endif // LOGGER_SETUP_HPP