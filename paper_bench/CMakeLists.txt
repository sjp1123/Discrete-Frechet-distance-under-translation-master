# Builds paper_bench against one arm of the tree:  cmake -DARM=<abs path to original|candidate5>
# The arm's own CMakeLists is pulled in as a subdirectory, so the driver links the very same
# `trans` objects and disc_arrangement_traversal library the arm's fut_paper_experiments uses.
cmake_minimum_required(VERSION 3.5)
project(paper_bench CXX)

if(NOT ARM)
	message(FATAL_ERROR "pass -DARM=<absolute path to original or candidate5>")
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

add_subdirectory(${ARM} arm)

# same compile/link flags the arm's CMakeLists.txt sets for its own targets
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic -std=c++14 -fopenmp -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp -fno-omit-frame-pointer")
add_definitions(-DNVERBOSE)
include_directories(${ARM}/src ${ARM}/lib/measurement_tool ${ARM}/lib/cgal_disk_arrangements)

add_executable(paper_bench paper_bench.cpp $<TARGET_OBJECTS:trans>)
set_target_properties(paper_bench PROPERTIES COMPILE_FLAGS "-DTRANSLATION")
target_link_libraries(paper_bench disc_arrangement_traversal)
