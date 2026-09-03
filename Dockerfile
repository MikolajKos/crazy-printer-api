# ================================================
# BUILDER (intermediate image)
# ================================================

FROM ubuntu:26.04 AS builder

# Avoids 'cannot initialize frontend' warning
ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /usr/src/app

# Download dependencies and remove apt packages lists
RUN apt-get update && apt-get install -y cmake g++ make ninja-build

COPY CMakeLists.txt .
COPY src/ src/
COPY include/ include/
COPY external/ external/
RUN mkdir build && cd build && cmake .. -G "Ninja"
RUN cd build && ninja

# ================================================
# MASTER IMAGE
# ================================================

FROM ubuntu:26.04 AS master_image

WORKDIR /usr/src/app

# Required for entrypoint bash script
RUN apt-get update && apt-get install -y gosu && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/src/app/build/CrazyPrinter ./

# Bash script that creates logs output directories with user 1000 privileges
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENV OUTPUT_BASE_DIR=/data/logs/

EXPOSE 8080

ENTRYPOINT ["/entrypoint.sh"]
# entrypoint.sh parameter
CMD ["./CrazyPrinter"]