#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""
pcp_watch.py – live view of order-0 MIGRATE_UNMOVABLE page allocations.

Examples
--------
sudo ./pcp_watch.py                 # all CPUs
sudo ./pcp_watch.py --cpu 2,5,7     # only CPUs 2,5,7
sudo ./pcp_watch.py --cpu 3 --sec 2 # CPU 3, 2-second window
"""

import argparse
import ctypes as ct
import time
from bcc import BPF

# ---------------------------------------------------------------------------#
# 1. command-line parsing                                                    #
# ---------------------------------------------------------------------------#
parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
parser.add_argument("--cpu", metavar="LIST",
                    help="comma/space-separated CPU IDs (default: all)")
parser.add_argument("--sec", type=float, default=1.0,
                    help="sampling interval in seconds (default: 1)")
args = parser.parse_args()

cpu_filter = None
if args.cpu is not None:
    # Accept both "1,3,5" and "1 3 5"
    cpu_filter = {int(tok) for part in args.cpu.split(',')
                  for tok in part.split() if tok.strip().isdigit()}
    if not cpu_filter:
        parser.error("no valid CPU id found in --cpu")

# ---------------------------------------------------------------------------#
# 2. eBPF program                                                            #
# ---------------------------------------------------------------------------#
BPF_SRC = r"""
#include <uapi/linux/ptrace.h>

BPF_HASH(cnt, u32, u64);          /* key = CPU, val = counter */

TRACEPOINT_PROBE(kmem, mm_page_alloc)
{
    if (args->order != 0 || args->migratetype != 1)   /* 1 == UNMOVABLE */
        return 0;

    u32 cpu = bpf_get_smp_processor_id();
    u64 zero = 0, *val;

    val = cnt.lookup_or_try_init(&cpu, &zero);
    if (val) (*val)++;
    return 0;
}
"""

bpf = BPF(text=BPF_SRC)
cnt = bpf["cnt"]

print("Counting …  Ctrl-C to stop.")
try:
    while True:
        time.sleep(args.sec)

        if cpu_filter is None:                        # --- all CPUs ----
            if cnt.items():
                print(f"\nΔ order-0 UNMOVABLE allocs per CPU "
                      f"({args.sec:g} s window)")
                for k, v in sorted(cnt.items(), key=lambda kv: kv[0].value):
                    print(f"cpu {k.value:<3} {v.value}")
            else:
                print(f"\n(no allocations in last {args.sec:g} s)")

        else:                                         # --- subset ------
            print(f"\nOrder-0 UNMOVABLE allocs / {args.sec:g} s")
            for cpu_id in sorted(cpu_filter):
                key = ct.c_uint(cpu_id)
                val = cnt[key] if key in cnt else None
                n   = val.value if val else 0
                print(f"cpu {cpu_id:<3} {n}")

        cnt.clear()
except KeyboardInterrupt:
    pass
