# ================================================
# BUILDER (intermediate image)
# ================================================

FROM ubuntu:26.04 AS builder

WORKDIR /usr/src/app

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

COPY --from=builder /usr/src/app/build/CrazyPrinter ./

EXPOSE 8080

USER 1000

CMD ["./CrazyPrinter"]