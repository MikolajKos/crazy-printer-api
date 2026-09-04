# 🖨️ Crazy Printer API — v1.0.0

> *"Because generating 2 Terabytes of fake logs shouldn't be boring."*

First stable release. The printer is operational, on fire, and ready to flood your filesystem.

---

## What is Crazy Printer API?

A high-performance, asynchronous mock log generator written in **C++20**. Built as a stress-testing tool for [LogGrid](https://github.com/MikolajKos/LogGrid.git) — a distributed log processing system — but useful for anyone who needs to hammer an I/O pipeline with gigabytes of realistic fake log data.

---

## 🚀 Quick Start

```bash
git clone https://github.com/MikolajKos/crazy-printer-api.git
cd crazy-printer-api
docker compose up -d --build

curl -X POST http://localhost:8080/api/printer/start \
  -H "Content-Type: application/json" \
  -d '{"fileCount": 100, "linesPerFile": 10000, "outputDir": "job_1", "producerThreads": 4, "consumerThreads": 4}'
```

---

## ✨ Features

### Core — Producer-Consumer pipeline

- **Async log generation** — `POST /api/printer/start` returns immediately with a job ID, generation runs in the background
- **Job status polling** — `GET /api/printer/status/:id` returns `filesWritten` counter in real time and `metrics.executionTimeSeconds` when done
- **ThreadSafeQueue with byte-based backpressure** — limits in-flight data to 256MB, not element count. No OOM surprises regardless of job size
- **Dual condition variables** — `m_cv_not_full` for producers, `m_cv_not_empty` for consumers. No thundering herd, no spurious wakeups
- **Exclusive file ownership** — each consumer thread owns its file exclusively. Zero file-level locking, zero disk thrashing

### Performance

- **Zero-allocation log generation** — `LogGenerator::GenerateLine` appends directly into a pre-reserved `std::string` batch. No per-line heap allocations across ~40M lines
- **`thread_local` RNG & distributions** — `mt19937` and `uniform_int_distribution` initialized once per thread, not per call
- **Single timestamp per batch** — `GetCurrentTimestamp()` called once per batch, not per line. Uses `format_to` into a `thread_local` buffer, returns `string_view` — zero allocations
- **Binary file write** — `std::ios::binary` + `file.write(data, size)` bypasses stream formatting overhead
- **Atomic work distribution** — producers claim lines via `fetch_add(batch_size)`, consumers claim files via `fetch_add(1)`. No scheduler, no mutex on the hot path
- **Measured throughput:** ~57 MB/s on HDD (8 producers, 4 consumers) — bottleneck is CPU, not disk

### Observability

- **Structured async logging via spdlog v1.15.3** — color output, thread IDs, millisecond timestamps
- **HTTP request logging** — every endpoint logs incoming request, response status, and latency in ms
- **Thread lifecycle logging** — producer/consumer thread start, shutdown, files written, queue exhaustion events
- **Runtime log level bound to build type** — `spdlog::set_level()` tied to compile-time `SPDLOG_ACTIVE_LEVEL`; Debug builds expose `DEBUG`/`TRACE`, Release builds show `INFO` and above only

### Developer experience

- **Configurable Docker build types:**

  ```bash
  BUILD_TYPE=Debug docker compose up -d --build            # full debug output
  BUILD_TYPE=Debug SAN_TYPE=ASAN docker compose up --build # AddressSanitizer
  BUILD_TYPE=Debug SAN_TYPE=TSAN docker compose up --build # ThreadSanitizer
  ```

- **Dependency Injection** — `PrinterController` depends on `IGeneratorService` interface, not the concrete implementation. Ready for unit testing with a mock
- **Path traversal protection** — `outputDir` leading slashes stripped automatically via `MakeRelative()`

---

## 🐛 Bugs fixed in this release

| Bug | Fix |
|---|---|
| Data race on `context->status` | Write moved into `MarkAsFinished()` called under `m_mutex` |
| Producer deadlock on full queue | `markDone()` now calls `notify_all` on `m_cv_not_full` |
| Debug logs silently dropped | `spdlog::set_level()` now called in `LoggerSetup::Init()` |
| TSAN data race in `GetCurrentTimestamp` | `time_zone` lazy init warmed up in `GeneratorService` constructor before any threads start |
| TSAN crash in Docker (`personality` syscall blocked) | `security_opt: seccomp=unconfined` added to `docker-compose.yaml` |

---

## 📦 Stack

| Component | Technology |
|---|---|
| Language | C++20 |
| HTTP Server | yhirose/cpp-httplib (vendored) |
| JSON | nlohmann/json v3.11.3 (vendored) |
| Logging | spdlog v1.15.3 (FetchContent) |
| Build | CMake 3.25+ |
| Container | Docker + Docker Compose |

---

## 🗺️ What's next

- `DELETE /api/printer/jobs/:id` — job cancellation via `std::jthread` stop token
- Input validation — limits on `fileCount`, `linesPerFile`, thread counts
- Throughput metrics — `bytesWritten` and `mbPerSecond` in `GET /status` response
- Unit tests — `MockGeneratorService` for controller-layer testing

---

*Low on magenta. Always.*
