/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "rubench.hpp"
#include "rubicon.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>


#define TARGET_OFFSET 0x10000UL

#define NR_VMA_LIMIT 63000UL
#define PAGE_TABLE_BACKED_SIZE 0x200000UL
#define SPRAY_START 0x100000000UL

static int fd_spray;
static void* file_ptr;
static unsigned long file_phys;

static void* pageblock;

static void* target;
static unsigned long target_phys;

void close_spraying_file() {
    munlock(file_ptr, PAGE_SIZE);
    munmap(file_ptr, PAGE_SIZE);
    close(fd_spray);
}

int spray_tables() {
    for(unsigned i = 0; i < NR_VMA_LIMIT; ++i) {
        void* addr = (void*)(SPRAY_START + PAGE_TABLE_BACKED_SIZE * i);
        if(mmap(addr, PAGE_SIZE, PROT_READ | PROT_WRITE,
                MAP_FIXED | MAP_SHARED | MAP_POPULATE, fd_spray,
                0) == MAP_FAILED) {
            printf("Failed to spray tables\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

void unspray_tables() {
    for(unsigned i = 1; i < NR_VMA_LIMIT; ++i) {
        void* addr = (void*)(SPRAY_START + PAGE_TABLE_BACKED_SIZE * i);
        if(munmap(addr, PAGE_SIZE)) {
            printf("Failed to unspray tables\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(void) {
    rubench_open();


    int num_rounds       = 100;
    long long total_time = 0;
    int num_fails        = 0;
    const char* buf = "ffffffffffffffff";

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);

        pageblock = get_page_block();

        fd_spray = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
        write(fd_spray, buf, 8);
        file_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE, fd_spray, 0);
        mlock(file_ptr, PAGE_SIZE);

        file_phys = rubench_va_to_pa(file_ptr);

        target = (void*)((unsigned long)pageblock + TARGET_OFFSET);
        mlock((void*)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);
        target_phys = rubench_va_to_pa(target);

        void* bait_ptr = (void*)((unsigned long)pageblock + PAGEBLOCK_SIZE / 2);
        migratetype_escalation(bait_ptr, 9, spray_tables);
        unspray_tables();

        munlock(target, PAGE_SIZE);
        block_merge(target, 0);
        mmap((void*)(SPRAY_START + PAGEBLOCK_SIZE), PAGE_SIZE,
             PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED | MAP_POPULATE,
             fd_spray,
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
        close_spraying_file();
        munlock((void*)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);
        munmap(pageblock, PAGEBLOCK_SIZE);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}