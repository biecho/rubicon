// SPDX-License-Identifier: MIT
#include "rubicon/pcp_evict.hpp"

#include <array>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

namespace rubicon {
namespace {

// ----- tiny RAII helpers ----------------------------------------------------
struct MMap {
    void* addr = MAP_FAILED;
    size_t len = 0;

    ~MMap() {
        if(addr != MAP_FAILED)
            ::munmap(addr, len);
    }
};

struct FdArray {
    std::vector<int> fds;

    ~FdArray() {
        for(int fd : fds)
            if(fd >= 0)
                ::close(fd);
    }
};

constexpr size_t kAnonBytes   = 2 * 1024 * 1024; // 2 MiB
constexpr size_t kNumTmpFiles = 1000;
constexpr auto kFill8         = "ffffffff"; // 8 B payload

} // anonymous namespace

// ---------------------------------------------------------------------------
// PUBLIC API
// ---------------------------------------------------------------------------
bool evict_pcp() noexcept {
    // 1. Touch 2 MiB of anonymous memory (prefaulted to get real pages).
    MMap area;
    area.len  = kAnonBytes;
    area.addr = ::mmap(nullptr,
                       area.len,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                       -1, 0);
    if(area.addr == MAP_FAILED)
        return false; // errno set by mmap

    // 2. Create tmp-files and dirty one cache-line in each.
    FdArray files;
    files.fds.reserve(kNumTmpFiles);

    for(size_t i = 0; i < kNumTmpFiles; ++i) {
        int fd = ::open("/tmp", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
        if(fd == -1)
            return false; // errno from open

        files.fds.push_back(fd);

        ssize_t wr = ::write(fd, kFill8, sizeof(kFill8));
        if(wr != static_cast<ssize_t>(sizeof(kFill8)))
            return false; // errno from write
    }

    // 3. Both RAII objects (area & files) go out of scope here, triggering
    //    munmap() + close() in their destructors  pages go back to the PCP
    return true;
}

} // namespace rubicon