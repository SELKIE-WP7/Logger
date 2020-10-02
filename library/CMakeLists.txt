# generate version.h
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h" @ONLY)

include_directories($<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

add_subdirectory(base)
add_subdirectory(GPS)
