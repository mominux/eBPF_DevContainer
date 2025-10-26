// apps/hello/hello.c
#define _GNU_SOURCE
#include <bpf/libbpf.h>
#include "hello.skel.h"

#include <dlfcn.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

static volatile sig_atomic_t exiting;

struct event {
    __u64 pid;
    __u64 size;
};

static void on_signal(int sig) { (void)sig; exiting = 1; }

static int bump_memlock(void) {
    struct rlimit r = { RLIM_INFINITY, RLIM_INFINITY };
    return setrlimit(RLIMIT_MEMLOCK, &r);
}

static int on_rb_event(void *ctx, void *data, size_t len) {
    (void)ctx;
    if (len < sizeof(struct event)) return 0;
    const struct event *e = data;
    printf("[ebpf] malloc(pid=%llu, size=%llu)\n",
           (unsigned long long)e->pid,
           (unsigned long long)e->size);
    fflush(stdout);
    return 0;
}

int main(void)
{
    int err = 0;
    struct hello_bpf *skel = NULL;
    struct ring_buffer *rb = NULL;

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if ((err = bump_memlock())) {
        fprintf(stderr, "setrlimit RLIMIT_MEMLOCK failed: %d\n", err);
        return 1;
    }

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);

    skel = hello_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "hello_bpf__open_and_load failed\n");
        return 1;
    }

    // Resolve malloc() in this process and its containing module
    void *sym = dlsym(RTLD_DEFAULT, "malloc");
    if (!sym) {
        fprintf(stderr, "dlsym(malloc) failed: %s\n", dlerror());
        err = 1; goto out;
    }

    Dl_info dinfo = {0};
    if (dladdr(sym, &dinfo) == 0 || !dinfo.dli_fname || !dinfo.dli_fbase) {
        fprintf(stderr, "dladdr() failed to resolve libc base\n");
        err = 1; goto out;
    }

    const char *lib_path = dinfo.dli_fname;
    unsigned long long base = (unsigned long long)(uintptr_t)dinfo.dli_fbase;
    unsigned long long addr = (unsigned long long)(uintptr_t)sym;
    unsigned long long off  = addr - base;

    // Attach uprobe to libc:malloc for *this* process only
    struct bpf_link *link = bpf_program__attach_uprobe(
        skel->progs.handle_malloc,
        /*retprobe=*/false,
        /*pid=*/getpid(),
        /*binary_path=*/lib_path,
        /*func_offset=*/off);
    if (!link) {
        err = -errno;
        fprintf(stderr, "attach_uprobe failed: %d\n", err);
        goto out;
    }
    skel->links.handle_malloc = link;

    // Ring buffer
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), on_rb_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "ring_buffer__new failed\n");
        err = 1; goto out;
    }

    printf("hello: attached to %s:malloc (pid=%d)\n", lib_path, getpid());
    printf("Triggering a few malloc() calls…\n");

    for (int i = 0; i < 5; i++) {
        void *p = malloc(32 + i);
        free(p);
        ring_buffer__poll(rb, 200);
    }

    printf("Polling; Ctrl-C to exit…\n");
    while (!exiting)
        ring_buffer__poll(rb, 250);

out:
    if (rb) ring_buffer__free(rb);
    if (skel) hello_bpf__destroy(skel);
    return err ? 1 : 0;
}