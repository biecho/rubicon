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
#include <unistd.h>
#include <sys/mman.h>

int main() {
    rubench_open();

    int num_rounds       = 100;
    long long total_time = 0;
    int num_fails        = 0;

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);

        void* pageblock = get_4mb_block();
        void* target    = (void*)((unsigned long)pageblock + TARGET_OFFSET);
        mlock((void*)(unsigned long)target, PAGE_SIZE);
        unsigned long target_phys = rubench_va_to_pa(target);

        const char* buf = "ffffffffffffffff";
        auto fd = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
        write(fd, buf, 8);
        auto fd_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_POPULATE, fd, 0);
        mlock(fd_ptr, PAGE_SIZE);

        auto addr    = (void*)(SPRAY_START + PAGEBLOCK_SIZE);
        pt_install(pageblock, target, addr, fd);

        unsigned long value = rubench_read_phys(target_phys);
        auto file_phys      = rubench_va_to_pa(fd_ptr);

        printf("Pageblock physical address: %lx\n", target_phys);
        printf("File physical address: %lx\n", file_phys);
        printf("Value read from target: %lx\n", value);
        auto success = (value & 0xFFFFFFFFF000) == file_phys;

        if(success) {
            printf("PASS\n");
        } else {
            printf("FAIL\n");
        }

        close(fd);
        munlock(fd_ptr, PAGE_SIZE);
        munmap(fd_ptr, PAGE_SIZE);

        munmap(addr, PAGE_SIZE);
        munmap(pageblock, PAGEBLOCK_SIZE);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}