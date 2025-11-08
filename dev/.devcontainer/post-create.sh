#!/usr/bin/env bash
set -euo pipefail

# Resolve workspace root relative to this script (no hardcoded paths)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WS_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "[post-create] WS_ROOT=${WS_ROOT}"

# Basic sanity (Dockerfile already installs these; this is just a guard)
command -v git >/dev/null 2>&1 || { echo "[post-create] git missing"; exit 1; }
command -v make >/dev/null 2>&1 || { echo "[post-create] make missing"; exit 1; }
command -v clang >/dev/null 2>&1 || { echo "[post-create] clang missing"; exit 1; }

# Clone libbpf-bootstrap if missing
if [ ! -d "${WS_ROOT}/libbpf-bootstrap" ]; then
  echo "[post-create] Cloning libbpf-bootstrap..."
  git clone --recurse-submodules https://github.com/libbpf/libbpf-bootstrap.git "${WS_ROOT}/libbpf-bootstrap"
else
  echo "[post-create] libbpf-bootstrap already present; skipping clone."
fi

C_EX="${WS_ROOT}/libbpf-bootstrap/examples/c"

# Always build the stable smoke test: minimal
echo "[post-create] Building 'minimal' smoke test…"
make -C "${C_EX}" -j1 minimal || { echo "[post-create] minimal build failed (non-fatal)"; }

# Optionally build 'profile' only if the source exists in this checkout
if [ -f "${C_EX}/profile.c" ]; then
  echo "[post-create] 'profile.c' found; trying to build 'profile'…"
  make -C "${C_EX}" -j1 profile || echo "[post-create] 'profile' build failed (ignored; example may be absent or changed upstream)"
else
  echo "[post-create] 'profile.c' not present in this libbpf-bootstrap version; skipping."
fi

# Bring your custom samples into libbpf-bootstrap/apps so they use the same build system
if [ -d "${WS_ROOT}/apps" ]; then
  echo "[post-create] Syncing your apps/ -> libbpf-bootstrap/apps/ …"
  rm -rf "${WS_ROOT}/libbpf-bootstrap/apps"
  cp -r "${WS_ROOT}/apps" "${WS_ROOT}/libbpf-bootstrap/"
  echo "[post-create] apps/ copied."
else
  echo "[post-create] No apps/ directory found in WS_ROOT; skipping copy."
fi

echo "[post-create] Done."
