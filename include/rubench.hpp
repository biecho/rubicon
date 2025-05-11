/*
* Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <linux/ioctl.h>

#define DEVICE_NAME "rubench"
#define CLASS_NAME "rubench_class"

#define RUBENCH_MAGIC 'R'

#define RUBENCH_GET_BLOCKS \
_IOR(RUBENCH_MAGIC, 1, struct rubench_get_blocks_data)
#define RUBENCH_VA_TO_PA _IOWR(RUBENCH_MAGIC, 2, struct rubench_va_to_pa_data)
#define RUBENCH_READ_PHYS _IOWR(RUBENCH_MAGIC, 3, struct rubench_read_phys_data)

struct rubench_get_blocks_data {
    unsigned long num_pages;
};

struct rubench_va_to_pa_data {
    void *va;
    unsigned long pa;
};

struct rubench_read_phys_data {
    unsigned long pa;
    unsigned long data;
};

void rubench_open();
void rubench_close();

int rubench_get_blocks();
unsigned long rubench_va_to_pa(void *va);
unsigned long rubench_read_phys(unsigned long pa);