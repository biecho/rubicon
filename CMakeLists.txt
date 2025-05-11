cmake_minimum_required(VERSION 3.16)
project(rubicon LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Global compile flags
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# 1. Library with the Rubicon user-space helper(s)
# ---------------------------------------------------------------------------
add_library(rubicon_pcp
        src/rubicon.cpp
        src/rubench.cpp
        src/rubench.hpp
        src/rubicon.hpp
        src/test_block_merge.cpp
)

# ---------------------------------------------------------------------------
# 2. Demo / example program
# ---------------------------------------------------------------------------
add_executable(test_pcp_evict
        src/test_pcp_evict.cpp
)
target_link_libraries(test_pcp_evict PRIVATE rubicon_pcp)

# optional install rule
install(TARGETS test_pcp_evict RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# 3. Convenience target to build the kernel module with Kbuild
#    (uses the Makefile sitting in kmod/)
# ---------------------------------------------------------------------------
add_custom_target(kmod
        COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/kmod
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/kmod
        COMMENT "Building kernel module rubench.ko with external Kbuild"
)