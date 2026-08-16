cmake_minimum_required(VERSION 3.25)
project(CrazyPrinterAPI CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(CrazyPrinter
    src/main.cpp
    src/GeneratorService.cpp
    src/PrinterController.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(CrazyPrinter
    PRIVATE
    Threads::Threads
)

target_include_directories(CrazyPrinter
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)