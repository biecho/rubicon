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
        src/page_block.cpp
        src/pt_spray.cpp
        src/pt_install.cpp
)

# ---------------------------------------------------------------------------
# 2. Demo / example program
# ---------------------------------------------------------------------------
add_executable(test_pcp_evict
        src/test_pcp_evict.cpp
)
target_link_libraries(test_pcp_evict PRIVATE rubicon_pcp)

add_executable(test_block_merge
        src/test_block_merge.cpp
)
target_link_libraries(test_block_merge PRIVATE rubicon_pcp)

add_executable(test_mtype_escalate
        src/test_mtype_escalate.cpp
)
target_link_libraries(test_mtype_escalate PRIVATE rubicon_pcp)

# ---------------------------------------------------------------------------
# 3. Convenience target to build the kernel module with Kbuild
#    (uses the Makefile sitting in kmod/)
# ---------------------------------------------------------------------------
add_custom_target(kmod
        COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/kmod
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/kmod
        COMMENT "Building kernel module rubench.ko with external Kbuild"
)

target_include_directories(rubicon_pcp
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>   # when built inside another project
        $<INSTALL_INTERFACE:include>                        # when installed
)

add_library(rubicon::rubicon_pcp ALIAS rubicon_pcp)         # modern alias
