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
#include <sys/mman.h>

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

        auto addr = (void*)(SPRAY_START + PAGEBLOCK_SIZE);
        auto pt_ctxt = pt_install(pageblock, target, addr);

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

        pt_deallocate(pt_ctxt);

        munmap(addr, PAGE_SIZE);
        munmap(pageblock, PAGEBLOCK_SIZE);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}