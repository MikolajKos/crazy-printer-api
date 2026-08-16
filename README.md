```
 ██████╗██████╗  █████╗ ███████╗██╗   ██╗
██╔════╝██╔══██╗██╔══██╗╚════██║╚██╗ ██╔╝
██║     ██████╔╝███████║    ██╔╝ ╚████╔╝ 
██║     ██╔══██╗██╔══██║   ██╔╝   ╚██╔╝  
╚██████╗██║  ██║██║  ██║   ██║     ██║   
 ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝     ╚═╝  
     ██████╗ ██████╗ ██╗███╗   ██╗████████╗███████╗██████╗
     ██╔══██╗██╔══██╗██║████╗  ██║╚══██╔══╝██╔════╝██╔══██╗
     ██████╔╝██████╔╝██║██╔██╗ ██║   ██║   █████╗  ██████╔╝
     ██╔═══╝ ██╔══██╗██║██║╚██╗██║   ██║   ██╔══╝  ██╔══██╗
     ██║     ██║  ██║██║██║ ╚████║   ██║   ███████╗██║  ██║
     ╚═╝     ╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝╚═╝  ╚═╝
                       🖨️  A P I  🔥
```

> *"Because generating 2 Terabytes of fake logs shouldn't be boring."*

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Status: WIP](https://img.shields.io/badge/Status-WIP%20(it's%20alive)-orange.svg)]()

---

## What is this?

**Crazy Printer API** is a high-performance, asynchronous mock log generator written in C++20. It floods your filesystem with gigabytes of hilariously fake log files — on purpose.

It was built as a stress-testing torture device for [LogGrid](https://github.com/MikolajKos/logrid) — a distributed log processing system. If LogGrid can survive Crazy Printer, it can survive anything.

Sample output, for the uninitiated:

```
[2026-08-16 03:14:15] [WARN]  Printer tray 2 is philosophically empty
[2026-08-16 03:14:16] [ERROR] Ink cartridge has achieved enlightenment and refuses to print
[2026-08-16 03:14:17] [FATAL] DEMONIC_POSSESSION detected in spooler process
[2026-08-16 03:14:18] [INFO]  Restarting printer... (attempt 666 of ∞)
```

---

## Architecture

Crazy Printer uses a **Producer-Consumer** pipeline with independent thread pools on both sides:

```
POST /api/printer/start
         │
         ▼
 GeneratorService::StartJob(config)
         │
         ├─► [Producer Pool]  →  generates text batches  →  [ThreadSafeQueue]
         │                                                          │
         └─► [Consumer Pool]  ←──────────────────────────  pops batches, writes to disk
```

### Key design decisions

- **Byte-based backpressure** — `ThreadSafeQueue` limits total in-flight data (default: 256MB), not element count. No OOM surprises.
- **Dual condition variables** — `m_cv_not_full` for producers, `m_cv_not_empty` for consumers. No thundering herd.
- **Exclusive file ownership** — each consumer thread owns its file exclusively. Zero file-level locking, zero disk thrashing.
- **Configurable thread counts** — tune `producerThreads` and `consumerThreads` independently for your hardware (HDD vs NVMe).
- **Dependency Injection** — `PrinterController` depends on `IGeneratorService`, not the concrete implementation. Testable by design.

---

## Stack

| Component | Technology |
|---|---|
| Language | C++20 |
| HTTP Server | [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) |
| Build | CMake 3.25+ |
| Threads | `std::jthread`, `std::mutex`, `std::condition_variable`, `std::atomic` |

---

## API

### `POST /api/printer/start`

Starts an async log generation job. Returns immediately with a job ID — the printer does not wait for you.

**Request body:**
```json
{
  "fileCount": 100,
  "linesPerFile": 10000,
  "outputDir": "tmp/",
  "producerThreads": 4,
  "consumerThreads": 4
}
```

**Response `202 Accepted`:**
```json
{
  "jobId": 42,
  "status": "running"
}
```

---

### `GET /api/printer/status/:id`

Returns the current status of a job.

**Response `200 OK`:**
```json
{
  "jobId": 42,
  "status": "running"
}
```

**Response `404 Not Found`:**
```json
{
  "error": "Job not found. Did the printer eat it?"
}
```

---

## Building

```bash
cmake -S . -B build
cmake --build build
./build/CrazyPrinter
```

Requires a C++20-capable compiler (GCC 12+, Clang 14+).

---

## Project structure

```
crazy-printer-api/
├── external/
│   ├── httplib.h             # HTTP server (vendored)
│   └── json.hpp              # nlohmann/json (vendored)
├── include/
│   ├── IGeneratorService.hpp # Abstract interface (the contract)
│   ├── GeneratorService.hpp  # Concrete implementation (the chaos)
│   ├── ThreadPool.hpp        # Reusable thread pool
│   └── ThreadSafeQueue.hpp   # Byte-limited producer-consumer queue
└── src/
    ├── main.cpp              # Entry point
    ├── GeneratorService.cpp
    ├── PrinterController.cpp # HTTP layer
    └── ThreadPool.cpp
```

---

## Status

> 🚧 Work in progress. The printer is warming up.

- [x] Project structure & CMake setup
- [x] `IGeneratorService` interface & `GeneratorService` scaffold
- [x] `ThreadSafeQueue` with byte-based backpressure
- [x] `ThreadPool` with `std::jthread`
- [ ] `StartJob` — wire producers & consumers
- [ ] `PrinterController` — HTTP endpoints
- [ ] Humorous log content generator
- [ ] `main.cpp` — start the server

---

*Low on magenta. Always.*
