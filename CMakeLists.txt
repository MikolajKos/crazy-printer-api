cmake_minimum_required(VERSION 3.25)
project(CrazyPrinterAPI CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(CrazyPrinter
src/main.cpp
src/GeneratorService.cpp
src/PrinterController.cpp
src/ThreadPool.cpp
)

# ============================================================
# TARGET CONFIGURATION (compiler flags and macros)
# ============================================================

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(CrazyPrinter PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
    
    if (MSVC)
        target_compile_options(CrazyPrinter PRIVATE /W4)
    else()
        target_compile_options(CrazyPrinter PRIVATE -Wall -Wextra -Werror -Wpedantic -Wconversion)
    endif()
else() # Release
    target_compile_definitions(CrazyPrinter PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
    
    if (NOT MSVC)
        target_compile_options(CrazyPrinter PRIVATE -O3)
    endif()
endif()

# Sanitizers
if(NOT MSVC)
    if(CMAKE_SAN_TYPE STREQUAL "ASAN")
        target_compile_options(CrazyPrinter PRIVATE -fsanitize=address)
        target_link_options(CrazyPrinter PRIVATE -fsanitize=address -static-libasan)
    elseif(CMAKE_SAN_TYPE STREQUAL "TSAN")
        target_compile_options(CrazyPrinter PRIVATE -fsanitize=thread)
        target_link_options(CrazyPrinter PRIVATE -fsanitize=thread -static-libtsan)
    endif()
endif()


# ============================================================
# FETCHING EXTERNAL DEPENDENCIES
# ============================================================

# Fetch Logger (spdlog)
include(FetchContent)
FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.15.3.tar.gz
    URL_HASH SHA256=15a04e69c222eb6c01094b5c7ff8a249b36bb22788d72519646fb85feb267e67
)
FetchContent_MakeAvailable(spdlog)


# ============================================================
# LINKING
# ============================================================

find_package(Threads REQUIRED)

target_link_libraries(CrazyPrinter
    PRIVATE
    Threads::Threads
    spdlog::spdlog
)

target_include_directories(CrazyPrinter
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)