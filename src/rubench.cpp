/*
* Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "rubench.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

static int rubench_fd = -1;

void rubench_open() {
    rubench_fd = open("/dev/" DEVICE_NAME, O_RDWR);
    if(rubench_fd < 0) {
        printf("Failed to open /dev/rubench\n");
        exit(EXIT_FAILURE);
    }
}

void rubench_close() { close(rubench_fd); }

int rubench_get_blocks() {
    struct rubench_get_blocks_data data_struct;

    if(ioctl(rubench_fd, RUBENCH_GET_BLOCKS, &data_struct) < 0) {
        printf("Failed to get blocks\n");
        exit(EXIT_FAILURE);
    }

    return data_struct.num_pages;
}

unsigned long rubench_va_to_pa(void* va) {
    struct rubench_va_to_pa_data data_struct;
    data_struct.va = va;

    if(ioctl(rubench_fd, RUBENCH_VA_TO_PA, &data_struct) < 0) {
        printf("Failed to convert VA to PA\n");
        exit(EXIT_FAILURE);
    }

    return data_struct.pa;
}

unsigned long rubench_read_phys(unsigned long pa) {
    struct rubench_read_phys_data data_struct;
    data_struct.pa = pa;

    if(ioctl(rubench_fd, RUBENCH_READ_PHYS, &data_struct) < 0) {
        printf("Failed to read physical memory\n");
        exit(EXIT_FAILURE);
    }

    return data_struct.data;
}

long long time_round(void (*func)(void)) {
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    func();

    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("Start time: %ld.%ld\n", start.tv_sec, start.tv_nsec);
    printf("End time: %ld.%ld\n", end.tv_sec, end.tv_nsec);

    return (long long)(end.tv_sec - start.tv_sec) * 1000000000 +
        (end.tv_nsec - start.tv_nsec);
}

void run_microbenchmark(int num_rounds,
                        void (*pre)(void),
                        void (*func)(void),
                        int (*post)(void)) {
    long long total_time = 0;
    int num_fails        = 0;

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);
        pre();

        long long round_time = time_round(func);

        if(post()) {
            num_fails++;
            printf("FAIL\n");
        } else {
            total_time += round_time;
            printf("PASS: %lld ns\n", round_time);
        }
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));
}