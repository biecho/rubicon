#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#define PAGE_SIZE 0x1000UL
#define PCP_PUSH_SIZE 0x2000000UL

// Size of the virtual-address range covered by one x86-64 4 KiB page table
inline constexpr std::size_t kX86_64PageTableSpan = 1ULL << 21; // 2 MiB
inline constexpr std::size_t kPageBlockSize = 1ULL << 21; // 2 MiB

void* pt_install(const std::vector<void*>& bait_pages, void* pt_target, void* addr,
                 const std::vector<void*>& spray_pages, int fd_spray);

unsigned long exhaust_pages_size_bytes();
void* get_4mb_block(void* address);

bool is_page_aligned(uintptr_t addr) noexcept;
std::vector<void*> pages_in_span(void* base, std::size_t order);
void map_pages(const std::vector<void*>& pages, int fd);
void unmap_pages(const std::vector<void*>& pages);
std::vector<void*> strided_addresses(void* base,
                                     std::size_t count,
                                     std::size_t stride);

int pcp_evict();

void block_merge(void* target, unsigned order);
