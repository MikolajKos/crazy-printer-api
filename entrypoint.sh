#!/bin/bash
set -e

# Fetch default output dir from docker-compose
TARGET_DIR="${OUTPUT_BASE_DIR:-/data/logs}"

mkdir -p "$TARGET_DIR"

chown -R 1000:1000 "$TARGET_DIR"

exec gosu 1000:1000 "$@"