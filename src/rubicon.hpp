/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 * 
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <cstdio>

#define PAGE_SIZE 0x1000UL
#define PAGEBLOCK_SIZE 0x200000UL

// Size of the virtual-address range covered by one x86-64 4 KiB page table
inline constexpr std::size_t kX86_64PageTableSpan = 1ULL << 21; // 2 MiB

#define TARGET_OFFSET 0x10000UL

#define NR_PAGE_TABLES_SPRAY 63000UL
#define SPRAY_START 0x100000000UL

void* pt_install(void* bait_ptr, void* pt_target, void* addr, int fd);

struct pt_spray_args_t {
    void* start;
    int fd;
    std::size_t nr_tables;
};

int pt_spray_tables(const pt_spray_args_t& args);
int pt_unspray_tables(const pt_spray_args_t& args);


void* get_4mb_block();


int pcp_evict();

int block_merge(void* target, unsigned order);

int migratetype_escalation(void* bait,
                           unsigned bait_order,
                           int (*bait_allocator)(void*),
                           void* alloc_ctx);