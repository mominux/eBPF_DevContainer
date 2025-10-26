# apps/hello/Makefile — standalone build using in-tree libbpf & bpftool

APP          := hello
OUTPUT       := .output
BPF_OBJ      := $(OUTPUT)/$(APP).bpf.o
SKEL_HDR     := $(OUTPUT)/$(APP).skel.h
USER_OBJ     := $(OUTPUT)/$(APP).o
USER_BIN     := $(APP)

# ---- arch for BPF target ----
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
  BPF_ARCH := x86
else ifeq ($(UNAME_M),aarch64)
  BPF_ARCH := arm64
else
  BPF_ARCH := $(UNAME_M)
endif

# ---- tools ----
CLANG ?= clang
CC    ?= gcc
MAKE  ?= make

# ---- in-tree libbpf & bpftool (relative to this app dir) ----
LIBBPF_SRC      := $(abspath ../../libbpf/src)
LIBBPF_BUILD    := $(abspath $(OUTPUT)/libbpf-build)   # <— build here
LIBBPF_DEST     := $(abspath $(OUTPUT)/libbpf)         # <— install here
LIBBPF_OBJ      := $(LIBBPF_DEST)/libbpf.a

BPFTOOL_SRC     := $(abspath ../../bpftool/src)
BPFTOOL_OUTPUT  := $(abspath $(OUTPUT)/bpftool)
BPFTOOL         := $(BPFTOOL_OUTPUT)/bootstrap/bpftool

# headers installed by the local libbpf build + bundled UAPI headers
INCLUDE_LIBBPF  := -I$(abspath $(OUTPUT)/libbpf)          # so <bpf/bpf_helpers.h> resolves
INCLUDE_UAPI    := -I$(abspath ../../libbpf/include/uapi)

# Clang’s default system include dirs for -target bpf
CLANG_BPF_SYS_INCLUDES := $(shell $(CLANG) -v -E - </dev/null 2>&1 | \
  sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p }')
# ---- sources ----
BPF_SRC   := $(APP).bpf.c
USER_SRC  := $(APP).c

# ---- flags ----
BPF_CFLAGS  := -D__TARGET_ARCH_$(BPF_ARCH) -I$(CURDIR) \
               $(INCLUDE_LIBBPF) $(INCLUDE_UAPI) \
               $(CLANG_BPF_SYS_INCLUDES) \
               -O2 -g -target bpf

USER_CFLAGS := -O2 -g -I$(OUTPUT) $(INCLUDE_LIBBPF) $(INCLUDE_UAPI)
USER_LDLIBS := -L$(LIBBPF_DEST) -Wl,-rpath,$(LIBBPF_DEST) -lbpf -lelf -lz

.PHONY: all clean run
all: $(USER_BIN)

# --- dirs ---
$(OUTPUT):
	@mkdir -p $@

$(BPFTOOL_OUTPUT):
	@mkdir -p $@

# --- build & "install" libbpf locally (build → LIBBPF_BUILD, install → LIBBPF_DEST) ---
$(LIBBPF_OBJ): | $(OUTPUT)
	@echo "  LIB      $(LIBBPF_OBJ)"
	@$(MAKE) -C $(LIBBPF_SRC) BUILD_STATIC_ONLY=1 \
		OBJDIR=$(LIBBPF_BUILD) \
		DESTDIR=$(LIBBPF_DEST) \
		INCLUDEDIR= LIBDIR= UAPIDIR= install > /dev/null

# --- build bpftool (bootstrap) locally ---
$(BPFTOOL): | $(BPFTOOL_OUTPUT)
	@echo "  BPFTOOL  $(BPFTOOL)"
	@$(MAKE) ARCH= CROSS_COMPILE= OUTPUT=$(BPFTOOL_OUTPUT)/ -C $(BPFTOOL_SRC) bootstrap > /dev/null

# 1) compile BPF C -> .bpf.o (needs headers, so depend on libbpf install)
$(BPF_OBJ): $(BPF_SRC) $(LIBBPF_OBJ) | $(OUTPUT)
	@echo "  BPF      $@"
	@$(CLANG) $(BPF_CFLAGS) -c $< -o $@

# 2) generate skeleton header from BPF obj (needs bpftool)
$(SKEL_HDR): $(BPF_OBJ) $(BPFTOOL) | $(OUTPUT)
	@echo "  GEN-SKEL $@"
	@$(BPFTOOL) gen skeleton $< > $@

# 3) userspace compile (depends on generated skeleton)
$(USER_OBJ): $(USER_SRC) $(SKEL_HDR) | $(OUTPUT)
	@echo "  CC       $@"
	@$(CC) $(USER_CFLAGS) -c $(USER_SRC) -o $@

# 4) link userspace with local libbpf.a
$(USER_BIN): $(USER_OBJ) $(LIBBPF_OBJ)
	@echo "  BINARY   $@"
	@$(CC) -o $@ $(USER_OBJ) $(USER_LDLIBS)

run: all
	@echo "Tip: in another shell you can run:  cat /sys/kernel/tracing/trace_pipe"
	./$(USER_BIN)

clean:
	@echo "  CLEAN"
	@rm -rf $(OUTPUT) $(USER_BIN)
