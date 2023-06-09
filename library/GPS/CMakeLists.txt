list(APPEND SL_GPS_SRC GPSCommands.c GPSMessages.c GPSSerial.c GPSTypes.c)
list(APPEND SL_GPS_INC GPSCommands.h GPSMessages.h GPSSerial.h GPSTypes.h)

add_library(SELKIELoggerGPS ${SL_GPS_SRC})
set_target_properties(SELKIELoggerGPS PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(SELKIELoggerGPS PROPERTIES SONAME 1)

set_target_properties(SELKIELoggerGPS PROPERTIES PUBLIC_HEADER SELKIELoggerGPS.h)
set_target_properties(SELKIELoggerGPS PROPERTIES PRIVATE_HEADER "${SL_GPS_INC}")

target_link_libraries(SELKIELoggerGPS PUBLIC SELKIELoggerBase)
include(GNUInstallDirs)

target_include_directories(SELKIELoggerGPS PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)
target_include_directories(SELKIELoggerGPS PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

install(TARGETS SELKIELoggerGPS
	EXPORT SLPubLibs
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT GPS
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger COMPONENT GPSDev
	PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger/GPS COMPONENT GPSDev
	)

set(CPACK_COMPONENT_GPS_GROUP lib)
set(CPACK_COMPONENT_GPS_DISPLAY_NAME "SELKIE GPS Library")
set(CPACK_COMPONENT_GPSDEV_GROUP dev)
set(CPACK_COMPONENT_GPSDEV_DISPLAY_NAME "SELKIE GPS Library - Headers")
set(CPACK_COMPONENT_GPSDEV_DEPENDS GPS)
