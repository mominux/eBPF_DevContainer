# eBPF DevContainer — Ubuntu 22.04 + libbpf-bootstrap + Custom Apps (`apps/hello`)

This repository provides a ready-to-run **VS Code Dev Container** for developing and testing **eBPF** programs using **clang**, **libbpf**, and **libbpf-bootstrap**.
It automatically builds example programs (`minimal`, `profile`, etc.) and supports your own applications inside the `apps/` directory (for example `apps/hello`) that demonstrate user-space **uprobes**, **ring buffers**, and clean attach/detach flows.

---

## Overview

The container offers a reproducible Ubuntu 22.04 environment for:

- Building and running **eBPF programs** via **libbpf-bootstrap**
- Using **bpftool**, **CO-RE**, and **systemd-BPF**
- Accessing host **kernel BTF**, **tracefs**, and **bpffs**
- Testing **tracepoints, uprobes, kprobes**, and **LSM hooks**
- Developing **custom apps** inside `/apps` (isolated from system noise)
- Supporting **symbolization** via **blazesym** (`BLAZESYM_ROOT=/host`)

Default bootstrap examples (`minimal`, `profile`) are built automatically when the container starts.

```bash
cd /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/examples/c
./minimal
```

Expected output:
```
Successfully started! Please run `cat /sys/kernel/tracing/trace_pipe` to see output...
```

---

## Container Configuration

The `.devcontainer/devcontainer.json` launches a privileged Ubuntu 22.04 container:

```json
"runArgs": [
  "--pid=host",
  "--privileged",
  "--cap-add=BPF",
  "--cap-add=PERFMON",
  "--cap-add=NET_ADMIN",
  "--cap-add=SYS_ADMIN",
  "--security-opt=seccomp=unconfined",
  "--ulimit", "memlock=-1:-1"
]
```

### Mounts

| Host Path | Container Path | Purpose |
|------------|----------------|----------|
| `/lib/modules` (ro) | same | Kernel headers/modules |
| `/sys/kernel/btf` (ro) | same | Kernel BTF for CO-RE |
| `/sys/kernel/tracing` (ro) | same | tracefs (ftrace output) |
| `/sys/kernel/debug` (ro) | same | Legacy tracefs fallback |
| `tmpfs` | `/sys/fs/bpf` | bpffs mount |
| `/` (ro) | `/host` | Used by blazesym for symbolization |

Environment:
```
BLAZESYM_ROOT=/host
```

---

## Automatic Setup

When the DevContainer is first created:

1. Installs dependencies (clang, llvm, libelf, cmake, etc.)
2. Clones **libbpf-bootstrap** if missing
3. Builds example programs:
   ```bash
   make -C libbpf-bootstrap/examples/c -j1 minimal profile
   ```
4. Installs VS Code extensions for C/C++ and CMake

Typical log:
```
make: 'minimal' is up to date.
make: 'profile' is up to date.
```

---

## Running libbpf-bootstrap Examples

### Minimal Tracepoint
```bash
cd /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/examples/c
./minimal
```

Then, in another terminal:
```bash
cat /sys/kernel/tracing/trace_pipe
```

Example output:
```
bpf_trace_printk: BPF triggered from PID 238188.
```

Generate more events:
```bash
echo hi >/tmp/x
printf 'zzz' >/dev/null
```

### Profile Example
```bash
cd /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/examples/c
make -j1 profile
./profile
```

---

## Custom Apps (`apps/hello`)

Your custom applications live in the `apps/` folder.
Before building, copy this folder into `libbpf-bootstrap/` so it can reuse the same Makefile structure:

```bash
cp -r /workspaces/eBPF_DevContainer/dev/apps /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/
cd /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/apps/hello
make -j1
```

### Running `apps/hello`

This program attaches a **user-space uprobe** to `libc:malloc` for the current process only, producing no system noise.
It sends allocation events through a ring buffer to user space.

**Run:**
```bash
cd /workspaces/eBPF_DevContainer/dev/libbpf-bootstrap/apps/hello
./hello
```

**Expected Output:**
```
hello: attached to /lib/x86_64-linux-gnu/libc.so.6:malloc (pid=452319)
Triggering a few malloc() calls…
[ebpf] malloc(pid=452319, size=8)
[ebpf] malloc(pid=452319, size=12)
[ebpf] malloc(pid=452319, size=32)
Polling; Ctrl-C to exit…
```

**Verify Attachment:**
```bash
../../bpftool/src/bpftool prog show
../../bpftool/src/bpftool link show
```

**Detach / Cleanup:**
```bash
# Normal exit:
Ctrl-C

# Force-detach:
bpftool link show
bpftool link detach id <LINK_ID>
```

---

## Included Tools

- clang / llvm 14
- libbpf & libelf-dev
- bpftool (built locally)
- cmake / make / pkg-config
- git / curl / vim / less
- VS Code extensions: C++, CMake Tools, and CMake Syntax

---

## Verification

```bash
mount | grep -E 'tracefs|bpffs'
test -r /sys/kernel/btf/vmlinux && echo "BTF OK" || echo "BTF MISSING"
bpftool prog show
bpftool map show
```

---

## Troubleshooting

| Problem | Fix |
|----------|-----|
| `sudo: command not found` | You are root; no sudo needed. |
| `trace_pipe: No such file` | Mount on host: `sudo mount -t tracefs tracefs /sys/kernel/tracing`. |
| Missing bpftool | Build with `make -C libbpf-bootstrap/examples/c bpftool`. |
| “directory not in bpffs” | `umount /sys/fs/bpf && mount -t bpf bpf /sys/fs/bpf`. |
| Container exits (137) | Low memory; build with `-j1`. |

---

## Notes

- The container runs **privileged** to allow eBPF loading and trace attachment.
- No need for `sudo` inside the container.
- Both `/sys/kernel/tracing` and `/sys/kernel/debug` are mounted for ftrace access.
- The `apps/` folder can host more examples in the future (e.g., `firewall_bpf`, `io_monitor`).
- Extend by copying and modifying `minimal.c` or `hello.c` to test new hooks.
