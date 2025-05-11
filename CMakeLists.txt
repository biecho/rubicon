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
        src/pcp_evict.cpp
)

target_include_directories(rubicon_pcp
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/uapi>
        $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# 2. Demo / example program
# ---------------------------------------------------------------------------
add_executable(rubicon_pcp_demo
        examples/example.cpp
)
target_link_libraries(rubicon_pcp_demo PRIVATE rubicon_pcp)

# optional install rule
install(TARGETS rubicon_pcp_demo RUNTIME DESTINATION bin)

# ---------------------------------------------------------------------------
# 3. Convenience target to build the kernel module with Kbuild
#    (uses the Makefile sitting in kmod/)
# ---------------------------------------------------------------------------
add_custom_target(kmod
        COMMAND ${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/kmod
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/kmod
        COMMENT "Building kernel module rubench.ko with external Kbuild"
)