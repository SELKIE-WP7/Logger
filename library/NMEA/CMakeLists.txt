list(APPEND SL_NMEA_SRC NMEASerial.c NMEAMessages.c)
list(APPEND SL_NMEA_INC NMEASerial.h NMEATypes.h NMEAMessages.h)

add_library(SELKIELoggerNMEA ${SL_NMEA_SRC})
set_target_properties(SELKIELoggerNMEA PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(SELKIELoggerNMEA PROPERTIES SONAME 1)

set_target_properties(SELKIELoggerNMEA PROPERTIES PUBLIC_HEADER SELKIELoggerNMEA.h)
set_target_properties(SELKIELoggerNMEA PROPERTIES PRIVATE_HEADER "${SL_NMEA_INC}")

target_link_libraries(SELKIELoggerNMEA PUBLIC SELKIELoggerBase)

include(GNUInstallDirs)

target_include_directories(SELKIELoggerNMEA PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)
target_include_directories(SELKIELoggerNMEA PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

install(TARGETS SELKIELoggerNMEA
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger
	PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger/NMEA
	)