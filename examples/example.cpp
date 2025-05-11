#include <rubicon/pcp_evict.hpp>
#include <rubench.h>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>

int main()
{
    // 2. Query kernel module
    int fd = open("/dev/rubench", O_RDONLY);
    if (fd == -1) { perror("open /dev/rubench"); return 1; }

    rubench_get_blocks_data d{};
    if (ioctl(fd, RUBENCH_GET_BLOCKS, &d) == -1) {
        perror("ioctl");
        return 1;
    }

    std::cout << "PCP list holds " << d.num_pages << " pages before PCP eviction\n";


    pcp_evict();


    if (ioctl(fd, RUBENCH_GET_BLOCKS, &d) == -1) {
        perror("ioctl");
        return 1;
    }
    close(fd);

    std::cout << "PCP list holds " << d.num_pages << " pages after PCP eviction\n";

}
