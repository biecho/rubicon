/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 * 
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "rubicon.hpp"

#include <cstdint>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

int pcp_evict() {
    void* flush_ptr = mmap(NULL, PCP_PUSH_SIZE, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE, -1, 0);
    if(flush_ptr == MAP_FAILED) {
        return -1;
    }

    return munmap(flush_ptr, PCP_PUSH_SIZE);
}

bool is_page_aligned(uintptr_t addr) noexcept {
    return (addr & (PAGE_SIZE - 1)) == 0;
}

std::vector<void*> strided_addresses(void* base,
                                     const std::size_t count,
                                     const std::size_t stride) {
    if(count == 0) {
        return {};
    }

    if(stride == 0) {
        throw std::invalid_argument("stride must be non-zero");
    }

    if(stride % PAGE_SIZE != 0) {
        throw std::invalid_argument("stride must be a multiple of PAGE_SIZE");
    }

    const auto start = reinterpret_cast<uintptr_t>(base);
    if(!is_page_aligned(start)) {
        throw std::invalid_argument("base address must be page-aligned");
    }

    std::vector<void*> addrs;
    addrs.reserve(count);

    for(std::size_t i = 0; i < count; ++i) {
        addrs.emplace_back(reinterpret_cast<void*>(start + stride * i));
    }

    return addrs;
}


std::vector<void*> pages_in_span(void* base, std::size_t order) {
    const auto first = reinterpret_cast<uintptr_t>(base);

    if(!is_page_aligned(first)) {
        throw std::invalid_argument("base address must be page-aligned");
    }

    const std::size_t span = PAGE_SIZE << order; // total bytes
    const uintptr_t last   = first + span;       // one-past-the-end

    std::vector<void*> pages;
    pages.reserve(span / PAGE_SIZE);

    for(uintptr_t p = first; p < last; p += PAGE_SIZE)
        pages.emplace_back(reinterpret_cast<void*>(p));

    return pages;
}

void map_pages(const std::vector<void*>& pages, int fd) {
    for(void* page : pages) {
        if(!is_page_aligned(reinterpret_cast<uintptr_t>(page))) {
            throw std::invalid_argument(
                "map_pages: address is not page-aligned");
        }

        const void* rv = mmap(page, PAGE_SIZE,
                        PROT_READ | PROT_WRITE,
                        MAP_FIXED | MAP_SHARED | MAP_POPULATE,
                        fd, 0);

        if(rv == MAP_FAILED) {
            throw std::system_error(errno, std::system_category(),
                                    "mmap failed");
        }
    }
}

void unmap_pages(const std::vector<void*>& pages) {
    for(void* p : pages) {
        if(!is_page_aligned(reinterpret_cast<uintptr_t>(p))) {
            throw std::invalid_argument(
                "unmap_pages: address is not page-aligned");
        }

        if(munmap(p, PAGE_SIZE) != 0) {
            throw std::system_error(errno, std::system_category(),
                                    "munmap failed");
        }
    }
}

void block_merge(void* target, unsigned order) {
    auto pages = pages_in_span(target, order);
    unmap_pages(pages);

    if(order != 0) {
        pcp_evict();
    }
}

unsigned long exhaust_pages_size_bytes() {
    return sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE) - 0x10000000UL;
}
