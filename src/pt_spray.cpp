#include "rubicon.hpp"

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>


int pt_spray_tables(const pt_spray_args_t& args) {
    for(unsigned i = 0; i < args.nr_tables; ++i) {
        void* addr = args.start + kX86_64PageTableSpan * i;

        if(mmap(addr, PAGE_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_FIXED | MAP_SHARED | MAP_POPULATE,
                args.fd, 0) == MAP_FAILED) {
            std::perror("spray_tables");
            return -1;
        }
    }
    return 0;
}

int pt_unspray_tables(const pt_spray_args_t& args) {
    for(unsigned i = 1; i < args.nr_tables; ++i) {
        void* addr = (void*)(args.start + kX86_64PageTableSpan * i);
        if(munmap(addr, PAGE_SIZE)) {
            printf("Failed to unspray tables\n");
            return -1;
        }
    }

    return 0;
}