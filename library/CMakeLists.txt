# generate version.h
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h" @ONLY)

include_directories($<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

add_subdirectory(base)
add_subdirectory(DW)
add_subdirectory(GPS)
add_subdirectory(LPMS)
add_subdirectory(MP)
add_subdirectory(MQTT)
add_subdirectory(NMEA)
add_subdirectory(N2K)
add_subdirectory(I2C)

include(CMakePackageConfigHelpers)
write_basic_package_version_file( "${CMAKE_CURRENT_BINARY_DIR}/SELKIELogger/SELKIELoggerConfigVersion.cmake" VERSION ${PROJECT_VERSION} COMPATIBILITY AnyNewerVersion)

export(EXPORT SLPubLibs FILE "${CMAKE_CURRENT_BINARY_DIR}/SELKIELogger/SELKIELoggerTargets.cmake" NAMESPACE SELKIELogger::)
configure_file(SELKIELoggerConfig.cmake "${CMAKE_CURRENT_BINARY_DIR}/SELKIELogger/SELKIELoggerConfig.cmake" COPYONLY)

set(ConfigPackageLocation lib/cmake/SELKIELogger)
install(EXPORT SLPubLibs FILE SELKIELoggerTargets.cmake NAMESPACE SELKIELogger:: DESTINATION ${ConfigPackageLocation})
install(FILES SELKIELoggerConfig.cmake "${CMAKE_CURRENT_BINARY_DIR}/SELKIELogger/SELKIELoggerConfigVersion.cmake" DESTINATION ${ConfigPackageLocation} COMPONENT BaseDev)

set(CPACK_COMPONENT_GROUP_LIBRARIES_DESCRIPTION "SELKIELogger Libraries")
set(CPACK_COMPONENT_GROUP_HEADERS_DESCRIPTION "SELKIELogger Headers (for development)")
