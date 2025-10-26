# eBPF DevContainer (Ubuntu 22.04 + libbpf-bootstrap + Hello Uprobe Sample)

This repository provides a ready‑to‑run **VS Code Dev Container** for developing, testing, and profiling **eBPF** programs using **libbpf**, **bpftool**, and **libbpf‑bootstrap**.  
It also includes self‑contained example apps — such as `apps/hello` and `apps/hello_uprobe` — demonstrating user‑space **uprobes**, **ring buffers**, and clean attach/detach flows.

---

## Overview

The DevContainer runs on Ubuntu 22.04 and is tuned for:
- Building eBPF programs with **clang + libbpf**
- Accessing **host kernel BTF** and `/sys/fs/bpf` from inside the container
- Supporting **user‑space symbolization** via **blazesym** and host debug symbols
- Running **libbpf‑bootstrap** examples (`minimal`, `profile`, etc.)
- Developing and testing **custom apps** like `hello_uprobe` (no system noise)

---

## Container Setup

The `.devcontainer/devcontainer.json` starts a privileged Ubuntu 22.04 container with:

- `--pid=host`, `--cap-add=BPF,PERFMON,NET_ADMIN,SYS_ADMIN`
- `--security-opt=seccomp=unconfined`
- `--ulimit memlock=-1:-1`

These enable loading BPF programs, pinning maps, and attaching to host PIDs.

**Mounts**
| Host Path | Container Path | Purpose |
|------------|----------------|----------|
| `/lib/modules` (ro) | same | Kernel headers/modules for the running kernel |
| `/sys/kernel/btf` (ro) | same | Kernel **BTF** for CO‑RE |
| `/sys/kernel/tracing` (ro) | same | Access to **ftrace/tracing** |
| tmpfs | `/sys/fs/bpf` | In‑container **bpffs** |
| `/` (ro) | `/host` | Used by **blazesym** for host binaries |
| `/usr/lib/debug` (ro) | same | Host debug symbols for user‑space stack resolution |

Environment:
```
BLAZESYM_ROOT=/host
```

---

## Prerequisites

- Docker (or compatible container runtime)  
- VS Code  
- Dev Containers extension (`ms-vscode-remote.remote-containers`)

Optional (on **host**):
```bash
sudo apt install libc6-dbg
# For tools like 'sleep' or 'ls':
# sudo apt install coreutils-dbgsym
```

---

## Quick Start

**Open in DevContainer**
```bash
git clone https://github.com/mominux/eBPF_DevContainer.git ebpf-dev
code ebpf-dev
```
VS Code → “**Reopen in Container**”

**Open a shell inside the container**
```bash
docker exec -it <container-id> /bin/bash
```

**Install build dependencies**
```bash
apt-get update
apt-get install -y build-essential clang llvm pkg-config cmake   libelf-dev zlib1g-dev libcap-dev libbfd-dev   libcurl4-openssl-dev libssl-dev dwarves git make curl ca-certificates
```

**Expose bpftool globally**
```bash
ln -sf /workspaces/dev/libbpf-bootstrap/examples/c/.output/bpftool/bootstrap/bpftool /usr/local/bin/bpftool
bpftool version
```

---

## Build and Run Examples

### Build minimal/profile samples
```bash
cd /workspaces/dev/libbpf-bootstrap/examples/c
make clean
make -j1 minimal
make -j1 profile
./profile
```

### Build the full bpftool (non‑bootstrap)
```bash
make -C /workspaces/dev/libbpf-bootstrap/bpftool/src -j
ln -sf /workspaces/dev/libbpf-bootstrap/bpftool/src/bpftool /usr/local/bin/bpftool
```

Verify:
```bash
bpftool prog show
bpftool map show
bpftool link show
```

---

## Sample App 1 — `apps/hello` (malloc uprobe)

This sample attaches a **userspace uprobe** to `libc:malloc` **for the current process only**, generating no system noise.  
It streams allocation events through a **ring buffer** to user‑space.

### Build
```bash
make -C apps/hello -j1
```

### Run
```bash
cd apps/hello
./hello
```

Expected output:
```
hello: attached to /lib/x86_64-linux-gnu/libc.so.6:malloc (pid=452319)
Triggering a few malloc() calls…
[ebpf] malloc(pid=452319, size=8)
[ebpf] malloc(pid=452319, size=12)
[ebpf] malloc(pid=452319, size=32)
...
Polling; Ctrl‑C to exit…
```

### Verify attachment
```bash
../../bpftool/src/bpftool prog show
../../bpftool/src/bpftool link show
```

### Detach / cleanup
```bash
# Normal
Ctrl‑C
# Force‑detach
bpftool link show
bpftool link detach id <LINK_ID>
```

### Debug tips
- Missing headers → rebuild: `make -C apps/hello -j1`
- To mount bpffs:
  ```bash
  mount -t bpf bpf /sys/fs/bpf || true
  ```
- To view verifier logs:
  ```c
  libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
  libbpf_set_print(libbpf_print_fn);
  ```

---

## Sample App 2 — `apps/hello_uprobe` (user‑space triggered)

A clean, minimal variant: the loader attaches to a symbol (`do_work`) in a small test binary.  
No kernel noise — it only fires when you run your `trigger` binary.

### Build
```bash
cd /workspaces/dev/libbpf-bootstrap/apps/hello_uprobe
make -j1
```

### Run
```bash
./hello_uprobe --bin ./trigger --symbol do_work
```

In another terminal:
```bash
./trigger
```

Expected output:
```
event: pid=12345 comm=trigger
event: pid=12345 comm=trigger
...
```

Stop with `Ctrl‑C`, or manually:
```bash
bpftool link detach id <LINK_ID>
```

---

## Optional: Helper Script

`tools/bpf-helpers.sh`
```bash
#!/usr/bin/env bash
set -euo pipefail
case "${1:-}" in
  ps) bpftool prog show; bpftool link show; bpftool map show ;;
  kill-link) bpftool link detach id "$2" || bpftool link delete id "$2" ;;
  remount-bpffs) umount /sys/fs/bpf || true; mount -t bpf bpf /sys/fs/bpf ;;
  *) echo "usage: $0 {ps|kill-link <ID>|remount-bpffs}" ;;
esac
```

---

## Health Checks

```bash
test -r /sys/kernel/btf/vmlinux && echo "BTF OK" || echo "BTF MISSING"
test -r /sys/kernel/tracing/available_tracers && echo "TRACEFS OK" || echo "TRACEFS MISSING"
mount | grep /sys/fs/bpf || echo "bpffs not mounted (tmpfs expected)"
echo "$BLAZESYM_ROOT"
test -r /host/usr/bin/ls && echo "HOST FS OK" || echo "HOST FS MISSING"
```

---

## Troubleshooting

| Problem | Fix |
|----------|-----|
| Container exits (137) | Out‑of‑memory → build with `-j1` or increase Docker memory |
| `sudo` not found | Use `apt-get` directly (you’re root) |
| Missing bpftool | Re‑link as shown above |
| “directory not in bpffs” | Ensure `/sys/fs/bpf` is mounted |
| Stale pinned object | `umount /sys/fs/bpf && mount -t bpf bpf /sys/fs/bpf` |

---

## One‑liner setup

```bash
apt-get update && apt-get install -y   build-essential clang llvm pkg-config cmake   libelf-dev zlib1g-dev libcap-dev libbfd-dev   libcurl4-openssl-dev libssl-dev dwarves git &&   ln -sf /workspaces/dev/libbpf-bootstrap/examples/c/.output/bpftool/bootstrap/bpftool /usr/local/bin/bpftool &&   bpftool version &&   cd /workspaces/dev/libbpf-bootstrap/examples/c &&   make clean && make -j1 minimal && make -j1 profile && ./profile
```

---

## Notes

- The container runs **privileged** with BPF and tracing capabilities.
- Everything builds **locally** in the repo — no system packages modified.
- Kernel BTF expected at `/sys/kernel/btf/vmlinux`.
- Contributions and extensions (e.g., `apps/firewall_bpf`, `apps/io_monitor`) welcome.

---

