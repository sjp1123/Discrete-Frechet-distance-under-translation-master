# candidate7 verification harnesses (from the 2026-09-26/27 audit of candidate6), built against one arm:
#   cmake -S paper_bench/c7_verify -B ~/b_c7v -DARM=<abs path to candidate7> -DCMAKE_BUILD_TYPE=RelWithDebInfo \
#         -DCMAKE_CXX_FLAGS="-include cstdint -include array -include cstddef"
# Needs GMP (gmpxx) for fuzz_oracle's exact oracle.
cmake_minimum_required(VERSION 3.5)
project(c7_verify CXX)
if(NOT ARM)
	message(FATAL_ERROR "pass -DARM=<abs path to candidate7>")
endif()
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()
add_subdirectory(${ARM} arm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -std=c++14 -fopenmp -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp -fno-omit-frame-pointer")
add_definitions(-DNVERBOSE)
include_directories(${ARM}/src ${ARM}/lib/measurement_tool ${ARM}/lib/cgal_disk_arrangements)

set(S ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROGS
	test_single_point_family ${ARM}/tests/test_single_point_family.cpp
	test_clustered_family    ${ARM}/tests/test_clustered_family.cpp
	test_exact_ref           ${S}/test_exact_ref.cpp
	test_one                 ${S}/test_one.cpp
	test_diag                ${S}/test_diag.cpp
	test_light               ${S}/test_light.cpp
	test_far                 ${S}/test_far.cpp
	fuzz_oracle              ${S}/fuzz_oracle.cpp
	probe                    ${S}/probe.cpp
	e2e                      ${S}/e2e.cpp
)
list(LENGTH PROGS n)
math(EXPR last "${n} - 1")
foreach(i RANGE 0 ${last} 2)
	math(EXPR j "${i} + 1")
	list(GET PROGS ${i} name)
	list(GET PROGS ${j} src)
	add_executable(${name} ${src} $<TARGET_OBJECTS:trans>)
	set_target_properties(${name} PROPERTIES COMPILE_FLAGS "-DTRANSLATION")
	target_link_libraries(${name} disc_arrangement_traversal gmpxx gmp)
endforeach()

add_executable(test_box_predicates ${ARM}/tests/test_box_predicates.cpp)
