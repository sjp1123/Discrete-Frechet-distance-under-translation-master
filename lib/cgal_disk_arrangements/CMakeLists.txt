find_package(CGAL QUIET COMPONENTS Core)

if (CGAL_FOUND)
	add_library(disc_arrangement_traversal SHARED
		disc_arrangement_traversal.cpp
		${CGAL_LIBRARIES}
		${CGAL_3RD_PARTY_LIBRARIES}
	)
	TARGET_LINK_LIBRARIES(disc_arrangement_traversal CGAL gmp mpfr measurement_tool)
else()
	message(STATUS "This program requires the CGAL library, and will not be compiled.")
endif()
