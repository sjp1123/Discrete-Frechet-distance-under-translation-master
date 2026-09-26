# Tests for candidate6.  Built against this arm the same way paper_bench builds
# against an arm (its `trans` objects + disc_arrangement_traversal):
#   cmake -S candidate6/tests -B ~/b_c6_tests -DCMAKE_BUILD_TYPE=RelWithDebInfo \
#         -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef"
#   cmake --build ~/b_c6_tests -j
# ARM defaults to the parent directory; pass -DARM=<abs path to candidate5|original>
# to run the same harnesses against another arm.
cmake_minimum_required(VERSION 3.5)
project(candidate6_tests CXX)

if(NOT ARM)
	get_filename_component(ARM "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

add_subdirectory(${ARM} arm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -std=c++14 -fopenmp -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp -fno-omit-frame-pointer")
add_definitions(-DNVERBOSE)
include_directories(${ARM}/src ${ARM}/lib/measurement_tool ${ARM}/lib/cgal_disk_arrangements)

foreach(t test_single_point_family test_clustered_family)
	add_executable(${t} ${t}.cpp $<TARGET_OBJECTS:trans>)
	set_target_properties(${t} PROPERTIES COMPILE_FLAGS "-DTRANSLATION")
	target_link_libraries(${t} disc_arrangement_traversal)
endforeach()

# Predicate unit test: includes maximal_regions.cpp directly (header-only Boost).
add_executable(test_box_predicates test_box_predicates.cpp)
