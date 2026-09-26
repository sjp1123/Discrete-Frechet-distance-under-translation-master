# USING cmake:
#   mkdir build
#   cd build
#   cmake ..
#   make -j

# RUN unit tests:
#   cd build
#   ctest



# OBJECT library requires 2.8.8
cmake_minimum_required(VERSION 3.5)
project(frechet CXX)

enable_testing()

include(CMakeDetermineCXXCompiler)

# default to RelWithDebInfo
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(EXTRA_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -pedantic" CACHE STRING "Extra flags used by the compiler during all build types.")
	set(EXTRA_EXE_LINKER_FLAGS "" CACHE STRING "Extra flags used by the linker.")
	set(EXTRA_EXE_LINKER_FLAGS_RELEASE "-flto" CACHE STRING "Extra flags used by the linker for the Release build type.")
	set(EXTRA_EXE_LINKER_FLAGS_RELWITHDEBINFO "-flto" CACHE STRING "Extra flags used by the linker for the RelWithDebInfo build type.")
else()
	set(EXTRA_CXX_FLAGS "" CACHE STRING "Extra flags used by the compiler during all build types.")
	set(EXTRA_EXE_LINKER_FLAGS "" CACHE STRING "Extra flags used by the linker.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS} -std=c++14 -fopenmp -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EXTRA_EXE_LINKER_FLAGS} -fopenmp -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${EXTRA_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} ${EXTRA_EXE_LINKER_FLAGS_RELWITHDEBINFO}")

# CGAL arrangement
add_subdirectory(lib/measurement_tool/)
include_directories(lib/measurement_tool//)

add_subdirectory(lib/cgal_disk_arrangements/)
include_directories(lib/cgal_disk_arrangements/)

option(VERBOSE "Verbose logging" OFF)
if(NOT VERBOSE)
	add_definitions(-DNVERBOSE)
endif()

SET(LIB_SOURCES
	src/discrete_frechet_light.cpp
	src/discrete_frechet_naive.cpp
	src/frechet_light.cpp
	src/frechet_naive.cpp
	src/geometry_basics.cpp
	src/filter.cpp
	src/orth_range_search.cpp
	src/parser.cpp
	src/query.cpp
	src/times.cpp
	src/curve.cpp
	src/curve_parameters.cpp
)

SET(LIB_SOURCES_TRANS
	src/discrete_frechet_light.cpp
	src/discrete_frechet_naive.cpp
	src/frechet_light.cpp
	src/frechet_naive.cpp
	src/geometry_basics.cpp
	src/filter.cpp
	src/orth_range_search.cpp
	src/parser.cpp
	src/query.cpp
	src/times.cpp
	src/fut_query.cpp
	src/frechet_under_translation.cpp
	src/curve_fut.cpp
	src/curve_parameters.cpp
	src/fut_n6_algorithm.cpp
	src/divide_and_conquer_vis.cpp
)

# compile shared sources only once, and reuse object files in both,
# as they are compiled with the same options anyway
add_library(common OBJECT ${LIB_SOURCES})
add_library(trans OBJECT ${LIB_SOURCES_TRANS})
add_library(all_flags OBJECT ${LIB_SOURCES_TRANS})
add_library(vis_cert OBJECT ${LIB_SOURCES})
add_library(cert OBJECT ${LIB_SOURCES})
set_target_properties(trans PROPERTIES COMPILE_FLAGS "-DTRANSLATION")
set_target_properties(all_flags PROPERTIES COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION")
set_target_properties(vis_cert PROPERTIES COMPILE_FLAGS "-DVIS -DCERTIFY")
set_target_properties(cert PROPERTIES COMPILE_FLAGS "-DCERTIFY")

add_executable(frechet
	src/main.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(frechet_under_translation
	src/main_fut.cpp
	$<TARGET_OBJECTS:trans>
)
set_target_properties(frechet_under_translation PROPERTIES
	COMPILE_FLAGS "-DTRANSLATION"
)
target_link_libraries(frechet_under_translation
	disc_arrangement_traversal
)

add_executable(run_tests
	src/certificate.cpp
	src/freespace_light_vis.cpp
	src/run_tests.cpp
	src/unit_tests.cpp
	$<TARGET_OBJECTS:all_flags>
)
set_target_properties(run_tests PROPERTIES
	COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION"
)
target_link_libraries(run_tests
	disc_arrangement_traversal
)

add_executable(test_curves
	src/certificate.cpp
	src/freespace_light_vis.cpp
	src/test_curves.cpp
	$<TARGET_OBJECTS:vis_cert>
)
set_target_properties(test_curves PROPERTIES
  COMPILE_FLAGS "-DVIS -DCERTIFY"
)

add_executable(pruning_progress
	src/certificate.cpp
	src/freespace_light_vis.cpp
	src/pruning_progress.cpp
	$<TARGET_OBJECTS:vis_cert>
)
set_target_properties(pruning_progress PROPERTIES
  COMPILE_FLAGS "-DVIS -DCERTIFY"
)

add_executable(export_freespace_diagram
	src/certificate.cpp
	src/export_freespace_diagram.cpp
	src/freespace_light_vis.cpp
	$<TARGET_OBJECTS:vis_cert>
)
set_target_properties(export_freespace_diagram PROPERTIES
  COMPILE_FLAGS "-DVIS -DCERTIFY"
)

add_executable(compare_implementations
	src/certificate.cpp
	src/compare_implementations.cpp
	$<TARGET_OBJECTS:cert>
)
set_target_properties(compare_implementations PROPERTIES
  COMPILE_FLAGS "-DCERTIFY"
)

add_executable(calc_frechet_distance
	src/calc_frechet_distance.cpp
	src/certificate.cpp
	src/freespace_light_vis.cpp
	$<TARGET_OBJECTS:vis_cert>
)
set_target_properties(calc_frechet_distance PROPERTIES
  COMPILE_FLAGS "-DVIS -DCERTIFY"
)

add_executable(calc_frechet_distance_under_translation
	src/calc_frechet_distance_under_translation.cpp
	src/freespace_light_vis.cpp
	src/certificate.cpp
	$<TARGET_OBJECTS:all_flags>
)
set_target_properties(calc_frechet_distance_under_translation PROPERTIES
  COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION"
)
target_link_libraries(calc_frechet_distance_under_translation
	disc_arrangement_traversal
)

add_executable(performance_test
	src/performance_test.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(paper_experiments
	src/paper_experiments.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(fut_paper_experiments
	src/fut_paper_experiments.cpp
	$<TARGET_OBJECTS:trans>
)
target_link_libraries(fut_paper_experiments
	disc_arrangement_traversal
)
set_target_properties(fut_paper_experiments PROPERTIES
	COMPILE_FLAGS "-DTRANSLATION"
)


add_executable(cert_experiments
	src/cert_experiments.cpp
	src/certificate.cpp
	$<TARGET_OBJECTS:cert>
)
set_target_properties(cert_experiments PROPERTIES
	COMPILE_FLAGS "-DCERTIFY"
)

add_executable(create_benchmark
	src/create_benchmark.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(create_benchmark_decider
	src/create_benchmark_decider.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(calc_all_distances
	src/calc_all_distances.cpp
	$<TARGET_OBJECTS:common>
)

add_executable(fut_create_benchmark_decider
	src/fut_create_benchmark_decider.cpp
	src/freespace_light_vis.cpp
	src/certificate.cpp
	$<TARGET_OBJECTS:all_flags>
)
set_target_properties(fut_create_benchmark_decider PROPERTIES
	COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION"
)
target_link_libraries(fut_create_benchmark_decider
	disc_arrangement_traversal
)

add_executable(fut_create_benchmark_decider_samechar
	src/fut_create_benchmark_decider_samechar.cpp
	src/freespace_light_vis.cpp
	src/certificate.cpp
	$<TARGET_OBJECTS:all_flags>
)
set_target_properties(fut_create_benchmark_decider_samechar PROPERTIES
	COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION"
)
target_link_libraries(fut_create_benchmark_decider_samechar
	disc_arrangement_traversal
)



add_executable(fut_create_benchmark_val_computation
	src/fut_create_benchmark_val_computation.cpp
	src/freespace_light_vis.cpp
	src/certificate.cpp
	$<TARGET_OBJECTS:all_flags>
)
set_target_properties(fut_create_benchmark_val_computation PROPERTIES
	COMPILE_FLAGS "-DVIS -DCERTIFY -DTRANSLATION"
)
target_link_libraries(fut_create_benchmark_val_computation
	disc_arrangement_traversal
)


