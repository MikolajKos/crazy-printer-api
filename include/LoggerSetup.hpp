#ifndef LOGGER_SETUP_HPP
#define LOGGER_SETUP_HPP

#include <string_view>

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
        
        // Set by compile options
        spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    }

    static void PrintWelcomeMessage() {
        static constexpr std::string_view logo = R"(
        
    ╔══════════════════════════════════════════════════════════════════════════╗
    ║  [ SYSTEM INITIATED: MASSIVE LOG GENERATION PROTOCOL ]                   ║
    ╠══════════════════════════════════════════════════════════════════════════╣
    ║                                                                          ║
    ║                                                                          ║
    ║                 ██████╗██████╗  █████╗ ███████╗██╗   ██╗                 ║
    ║                 ██╔════╝██╔══██╗██╔══██╗╚════██║╚██╗ ██╔╝                ║
    ║                 ██║     ██████╔╝███████║    ██╔╝ ╚████╔╝                 ║
    ║                 ██║     ██╔══██╗██╔══██║   ██╔╝   ╚██╔╝                  ║
    ║                 ╚██████╗██║  ██║██║  ██║   ██║     ██║                   ║
    ║                  ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝     ╚═╝                   ║
    ║          ██████╗ ██████╗ ██╗███╗   ██╗████████╗███████╗██████╗           ║
    ║          ██╔══██╗██╔══██╗██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗          ║
    ║          ██████╔╝██████╔╝██║██╔██╗ ██║   ██║   █████╗  ██████╔╝          ║
    ║          ██╔═══╝ ██╔══██╗██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗          ║
    ║          ██║     ██║  ██║██║██║ ╚████║   ██║   ███████╗██║  ██║          ║
    ║          ╚═╝     ╚═╝  ╚═╝╚═╝╚═╝   ╚═╝    ╚═╝   ╚══════╝╚═╝  ╚═╝          ║
    ║                                                                          ║
    ║    "Because generating 2 Terabytes of fake logs shouldn't be boring."    ║
    ║                                                                          ║
    ╠══════════════════════════════════════════════════════════════════════════╣
    ║  > ENGINE   : C++20 Asynchronous Multithreading Architecture             ║
    ║  > STATUS   : Ready to crush I/O limits                                  ║
    ╠══════════════════════════════════════════════════════════════════════════╣
    ║  > AUTHOR   : Mikołaj Kosiorek                                           ║
    ║  > GITHUB   : github.com/MikolajKos                                      ║
    ╚══════════════════════════════════════════════════════════════════════════╝
        )";

        fmt::print("{}\n", logo);
    }
};

#endif // LOGGER_SETUP_HPP