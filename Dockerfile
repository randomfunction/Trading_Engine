FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y g++ cmake git zlib1g-dev
WORKDIR /build
COPY . .
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y zlib1g && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /build/build/engine .
USER 1001
EXPOSE 8080
ENTRYPOINT ["./engine"]