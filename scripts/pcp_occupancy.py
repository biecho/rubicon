#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
pcp_occupancy.py – snapshot the current length of the order-0 /
                   MIGRATE_UNMOVABLE PCP list (same metric as
                   RUBENCH_GET_BLOCKS).

  sudo ./pcp_occupancy.py                 # all CPUs, 1-s refresh
  sudo ./pcp_occupancy.py --cpu 7,9 --sec 0.5
"""

import argparse, ctypes as ct, time
from bcc import BPF

# ---------- command-line ----------------------------------------------------
argp = argparse.ArgumentParser()
argp.add_argument("--cpu", help="comma/space list of CPU IDs")
argp.add_argument("--sec", type=float, default=1.0,
                  help="sampling interval in seconds (default 1s)")
args = argp.parse_args()

cpu_set = None
if args.cpu:
    cpu_set = {int(t) for seg in args.cpu.split(',') for t in seg.split()
               if t.strip().isdigit()}
    if not cpu_set:
        argp.error("no valid CPU id in --cpu")

# ---------- eBPF program ----------------------------------------------------
BPF_SRC = r"""
#include <linux/mmzone.h>
#include <linux/mm.h>

BPF_ARRAY(pcp_cnt, u64, 4096);     /* key = CPU id */

/* helper: return per-CPU pointer to pages cache */
static __always_inline
struct per_cpu_pages *get_pcp(int cpu)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,13,0)
    struct zone *z = &NODE_DATA(0)->node_zones[ZONE_NORMAL];
# ifdef CONFIG_SMP
    return per_cpu_ptr(z->per_cpu_pageset, cpu);
# else
    return &z->pageset.pcp;
# endif
#else
    struct zone *z = &NODE_DATA(0)->node_zones[ZONE_NORMAL];
    return per_cpu_ptr(z->per_cpu_pageset, cpu);
#endif
}

/* fire on each context-switch (high-rate but cheap TP) */
TRACEPOINT_PROBE(sched, sched_switch)
{
    int cpu = bpf_get_smp_processor_id();
    struct per_cpu_pages *pcp = get_pcp(cpu);

    u32 idx = order_to_pindex(MIGRATE_UNMOVABLE, 0);   /* order-0 list */
    u64 n   = READ_ONCE(pcp->count[idx]);

    pcp_cnt.update(&cpu, &n);
    return 0;
}
"""
bpf = BPF(text=BPF_SRC)
pcp_cnt = bpf["pcp_cnt"]

print("Live PCP occupancy (order-0 / UNMOVABLE) – Ctrl-C to stop")
try:
    while True:
        time.sleep(args.sec)
        cpus = cpu_set if cpu_set is not None else range(len(pcp_cnt))

        print()
        for c in sorted(cpus):
            key = ct.c_int(c)
            val = pcp_cnt[key]
            print(f"cpu {c:<3} {val.value}")
except KeyboardInterrupt:
    pass
