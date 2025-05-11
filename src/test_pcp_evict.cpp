#include "rubench.hpp"
#include "rubicon.hpp"

#include <iostream>

int main()
{
    rubench_open();

    auto blocks = rubench_get_blocks();

    std::cout << "PCP list holds " << blocks << " blocks before PCP eviction\n";

    pcp_evict();
    blocks = rubench_get_blocks();

    std::cout << "PCP list holds " << blocks << " blocks after PCP eviction\n";

    rubench_close();
}
