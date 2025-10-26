// apps/hello/hello.bpf.c
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

struct event {
    __u64 pid;
    __u64 size;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

SEC("uprobe")
int handle_malloc(struct pt_regs *ctx)
{
    struct event *e;
    __u64 sz = (__u64)PT_REGS_PARM1(ctx);
    __u64 pid = bpf_get_current_pid_tgid() >> 32;

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    e->pid  = pid;
    e->size = sz;
    bpf_ringbuf_submit(e, 0);
    return 0;
}
