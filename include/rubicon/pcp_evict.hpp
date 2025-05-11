#pragma once

namespace rubicon {

/**
 * Evict the per-CPU page-list (PCP cache) on the calling CPU.
 *
 * It allocates two transient memory pools:
 *   • 2 MiB anonymous RAM (prefaulted)
 *   • 1 000 one-page O_TMPFILE files
 *
 * Immediately releasing both pools overfills the PCP list,
 * forcing the kernel to drain it. After the call returns, the
 * next page allocation executed on the same CPU will come from the
 * global free list, not from a PCP list.
 *
 * @return true on success; false on system error
 */
bool evict_pcp() noexcept;

}