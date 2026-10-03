find_package(CGAL REQUIRED COMPONENTS Core)

add_library(disc_arrangement_traversal SHARED
    disc_arrangement_traversal.cpp
)

target_link_libraries(disc_arrangement_traversal
    CGAL::CGAL
    CGAL::CGAL_Core
    gmp
    mpfr
    measurement_tool
)
