# Install dependencies before running
# python3 -m venv .venv
# pip install -r benchmark/requirements.txt

# Run docker first with --tmpfs (RAM alocation) to test maximum throughput
'''
docker run -d --rm \
  --name crazy-printer \
  -p 8080:8080 \
  -e OUTPUT_BASE_DIR=/data/logs/ \
  --tmpfs /data/logs \
  ghcr.io/mikolajkos/crazy-printer-api:v1.0.0
'''

import requests
import time
import matplotlib.pyplot as plt

BASE_URL = "http://localhost:8080/api/printer"

# Config
file_count = 100
lines_per_file = 100000
total_lines = file_count * lines_per_file
total_mb = (total_lines * 100) / (1024 * 1024)

# Thread distribution between producer and consumer
scenarios = [
    (1, 1),
    (2, 2),
    (4, 4),
    (8, 2),
    (8, 8)
]

# Plot data lists
labels = []
speeds_mb = []

print(f"Task: {file_count} files, {lines_per_file}, lines. ({total_mb:.2f} MB)")
print("-" * 60)
print("| Producers | Consumers | Time (s) | Speed | Throughput |")

for prod, cons in scenarios:
    payload = {
        "fileCount": file_count,
        "linesPerFile": lines_per_file,
        "outputDir": f"bench_{prod}_{cons}",
        "producerThreads": prod,
        "consumerThreads": cons
    }
    
    # Request and Polling
    response = requests.post(f"{BASE_URL}/start", json=payload)
    response.raise_for_status()
    job_id = response.json()["jobId"]

    while True:
        status_res = requests.get(f"{BASE_URL}/status/{job_id}")
        status_data = status_res.json()

        if status_data["status"] == "done":
            exec_time = status_data["metrics"]["executionTimeSeconds"]

            lines_per_sec = total_lines / exec_time
            mb_per_sec = total_mb / exec_time

            # Save results
            labels.append(f"{prod}P / {cons}C")
            speeds_mb.append(mb_per_sec)

            print(f"| {prod} | {cons} | {exec_time:.3f} | {int(lines_per_sec):,} lines/s | {mb_per_sec:.2f} MB/s |")
            break

        time.sleep(0.5)

print("-" * 60)
print("Generating plot...")

# Plot configuration
plt.figure(figsize=(10,6))
bars = plt.bar(labels, speeds_mb, color='#00599C')

# Description
plt.title('Crazy Printer API - Throughput by Thread Configuration', fontsize=14, fontweight='bold')
plt.xlabel('Threads (Producers / Consumers)', fontsize=12)
plt.ylabel('Throughput (MB/s)', fontsize=12)
plt.grid(axis='y', linestyle='--', alpha=0.7)

for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + 1, f'{yval:.1f}', ha='center', va='bottom', fontweight='bold')

# Save results to a file
plt.savefig('benchmark_results.png', dpi=300, bbox_inches='tight')

print("Benchmark finished, results saved as 'benchmark_results.png'")