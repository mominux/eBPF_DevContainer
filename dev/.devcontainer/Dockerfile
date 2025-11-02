FROM ubuntu:22.04

# Noninteractive apt
ENV DEBIAN_FRONTEND=noninteractive

# Core toolchain and eBPF deps
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential make clang llvm pkg-config libelf-dev zlib1g-dev \
    git curl ca-certificates iproute2 linux-tools-common linux-tools-generic \
    vim less \
 && rm -rf /var/lib/apt/lists/*