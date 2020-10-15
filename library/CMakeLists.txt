# generate version.h
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h" @ONLY)

include_directories($<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

add_subdirectory(base)
add_subdirectory(GPS)
add_subdirectory(MP)
add_subdirectory(NMEA)
add_subdirectory(I2C)

set(CPACK_COMPONENT_GROUP_LIBRARIES_DESCRIPTION "SELKIELogger Libraries")
set(CPACK_COMPONENT_GROUP_HEADERS_DESCRIPTION "SELKIELogger Headers (for development)")
