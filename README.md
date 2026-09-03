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
[![Status: Ready](https://img.shields.io/badge/Status-Ready%20(it's%20alive%20%26%20screaming)-green.svg)]()

---

## What is this?

**Crazy Printer API** is a high-performance, asynchronous mock log generator written in C++20. It floods your filesystem with gigabytes of hilariously fake log files — on purpose.

It was built as a stress-testing torture device for [LogGrid](https://github.com/MikolajKos/LogGrid.git) — a distributed log processing system. If LogGrid can survive Crazy Printer, it can survive anything.

Sample output, for the uninitiated:

```
[2026-08-16 03:14:15] [WARN]  Printer tray 2 is philosophically empty
[2026-08-16 03:14:16] [ERROR] Ink cartridge has achieved enlightenment and refuses to print
[2026-08-16 03:14:17] [FATAL] DEMONIC_POSSESSION detected in spooler process
[2026-08-16 03:14:18] [INFO]  Restarting printer... (attempt 666 of ∞)
```

---

## Quick Start

Starting the entire architecture takes only seconds:

1. Clone the repository.
2. Ensure you have Docker and Docker Compose installed.
3. Spin up the Crazy Printer using:
   ```bash
   docker compose up -d --build
   ```
4. To view the live, aggregated output of the printer:
   ```bash
   docker compose logs -f
   ```

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
> **📝 Note:** The `"outputDir"` field defines a sub-directory within the base logs directory (which is set to `/data/logs` by default via the environment). For example, providing `"outputDir": "job_123"` will automatically save the generated files to `/data/logs/job_123` on the mounted Docker volume.

```json
{
  "fileCount": 100,
  "linesPerFile": 10000,
  "outputDir": "output",
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
│   ├── GeneratorService.hpp  # Interface + concrete implementation + data structs
│   ├── PrinterController.hpp # HTTP layer (forward decl only, no httplib include)
│   ├── ThreadPool.hpp        # Reusable thread pool
│   ├── LogGenerator.hpp      # Log line generator (zero-alloc append)
│   └── ThreadSafeQueue.hpp   # Byte-limited producer-consumer queue
└── src/
    ├── main.cpp              # Entry point
    ├── GeneratorService.cpp  # Job lifecycle, producer & consumer tasks
    ├── PrinterController.cpp # HTTP endpoints
    └── ThreadPool.cpp        # jthread pool impl
```

---

## Status

> ✅ Fully operational. The printer is on fire (intentionally).

- [x] Project structure & CMake setup
- [x] `ThreadSafeQueue` with byte-based backpressure & `markDone()` wakeup for both sides
- [x] `ThreadPool` with `std::jthread`
- [x] `LogGenerator` — zero-allocation line append, `thread_local` RNG & distributions
- [x] `StartJob` — producers & consumers wired up, output dir prep, atomic file ownership
- [x] `PrinterController` — `POST /start` and `GET /status/:id` endpoints
- [x] `main.cpp` — server starts on `0.0.0.0:8080`
- [x] Data race on `status` field fixed (`m_mutex` guards both read and write)
- [x] Producer deadlock fixed (`markDone()` now wakes blocked producers too)

---

*Low on magenta. Always.*
