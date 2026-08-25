#ifndef LOG_GENERATOR_HPP
#define LOG_GENERATOR_HPP

#include <array>
#include <chrono>
#include <format>
#include <random>
#include <string_view>
#include <string>

namespace LogGenerator {
    constexpr auto LEVELS = std::to_array<std::string_view>({
        "WARN",
        "ERROR",
        "DEMONIC_POSSESSION",
        "INFO"
    });
    
    constexpr auto MESSAGES = std::to_array<std::string_view>({
        "Printer tray 2 is philosophically empty.",
        "Ink cartridge has achieved enlightenment and refuses to print.",
        "Printer is currently questioning the conceptual nature of 'paper'.",
        "Toner level low. Toner self-esteem even lower.",
        "Cyan ink is feeling melancholy today. Refusing to mix with Yellow.",
        "Error 404: Motivation to print not found.",
        "Scanning for documents... found only existential dread.",
        "Printing blank pages to make a statement about modern art.",
        "Document successfully printed, but at what cost?",

        "DEMONIC_POSSESSION detected in spooler process.",
        "Restarting printer... (attempt 666 of ∞)",
        "Firmware corrupted by ancient Sumerian curse. Please consult an exorcist.",
        "Warming up... overheating... opening portal to the underworld.",
        "The magenta cartridge demands a blood sacrifice.",
        "Paper jam detected in the 4th dimension.",
        "Poltergeist identified in the network card. Disconnecting...",
        "Roller 3 is spinning counter-clockwise to reverse time.",

        "Spooler service has unionized and is demanding paid time off.",
        "Magenta cartridge has declared independence from the motherboard.",
        "Print job 'resume.pdf' rejected due to lack of ambition.",
        "User pressed 'Cancel' too aggressively. Printer is now plotting revenge.",
        "Network handshake failed. Printer refuses to shake hands with a cheap router.",
        "Replacing fuser unit with a small, angry dragon.",
        "Driver update failed: OS is scared of the new driver.",
        "Printer is asleep. Do not disturb the printer.",

        "Detected unauthorized cheese inside the paper tray.",
        "Ink levels are adequate, but the vibe is completely off.",
        "Paper tray 1 is missing. Paper tray 1 never existed. Wake up.",
        "Out of paper. Please insert ancient parchment.",
        "Print queue is currently experiencing a temporal paradox."
    });

    inline void GenerateLine(std::string_view timestamp, std::string& batch) {
        thread_local std::mt19937 rng{std::random_device{}()};
        thread_local static std::uniform_int_distribution<size_t> lvl_dist(0, LEVELS.size() - 1);
        thread_local static std::uniform_int_distribution<size_t> msg_dist(0, MESSAGES.size() - 1);

        const size_t lvl_index = lvl_dist(rng);
        const size_t msg_index = msg_dist(rng);

        // Zero-allocation direct append for maximum performance (bypasses format parsing)
        batch.append("[");
        batch.append(timestamp);
        batch.append("] [");
        batch.append(LEVELS[lvl_index]);
        batch.append("] ");
        batch.append(MESSAGES[msg_index]);
        batch.append("\n");
    }

    // Safe to return string_view because 'buffer' is thread_local and outlives the function scope.
    inline std::string_view GetCurrentTimestamp() {
        static const auto* tz = std::chrono::current_zone();

        thread_local std::string buffer;
        buffer.clear();

        const auto now = std::chrono::system_clock::now();
        const auto local_time = tz->to_local(now);

        std::format_to(std::back_inserter(buffer), "{:%Y-%m-%d %H:%M:%S}", local_time);

        return buffer;
    }
}

#endif // LOG_GENERATOR_HPP