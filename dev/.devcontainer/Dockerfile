FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# Tools & deps (no bpftool from apt)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      ca-certificates curl git make gcc clang llvm \
      build-essential pkg-config \
      libelf-dev zlib1g-dev libcap-dev libssl-dev \
      flex bison libncurses5-dev wget xz-utils \
      python3 python3-venv file && \
    update-ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Rust for blazesym-capable builds at runtime
ENV CARGO_HOME=/root/.cargo \
    RUSTUP_HOME=/root/.rustup \
    PATH=/root/.cargo/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
RUN curl -sSf https://sh.rustup.rs | sh -s -- -y && \
    rustup toolchain install stable && rustup default stable

# Build a recent bpftool (has `gen skeleton`)
RUN git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git /tmp/linux-kernel && \
    make -C /tmp/linux-kernel/tools/lib/bpf -j"$(nproc)" BUILD_STATIC_ONLY=y && \
    make -C /tmp/linux-kernel/tools/bpf/bpftool -j"$(nproc)" && \
    install -m 0755 /tmp/linux-kernel/tools/bpf/bpftool/bpftool /usr/local/sbin/ && \
    rm -rf /tmp/linux-kernel

WORKDIR /workspaces/dev
