set(DEFAULT_MON_PREFIX "mon" CACHE STRING "Define the default prefix used for the logger output files")
set(DEFAULT_STATE_NAME "SLogger.state" CACHE STRING "Define the default state file name")
set(DEFAULT_MARK_FREQUENCY 10 CACHE STRING "Default local timestamp frequency")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Logger.h.in" "${CMAKE_CURRENT_BINARY_DIR}/Logger.h" @ONLY)

include(GNUInstallDirs)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} PRIVATE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE)

add_executable(Logger Logger.h Logger.c LoggerConfig.c LoggerDMap.c LoggerGPS.c LoggerSignals.c LoggerMP.c LoggerMQTT.c LoggerNet.c LoggerNMEA.c LoggerI2C.c LoggerSerial.c LoggerTime.c)
target_link_libraries(Logger PUBLIC Threads::Threads)
target_link_libraries(Logger PUBLIC SELKIELoggerGPS SELKIELoggerBase SELKIELoggerMP SELKIELoggerMQTT SELKIELoggerNMEA SELKIELoggerI2C)
target_link_libraries(Logger PUBLIC inih)

install(TARGETS Logger RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Logger)

set(CPACK_COMPONENT_LOGGER_GROUP "Applications")
set(CPACK_COMPONENT_LOGGER_DESCRIPTION "Main data recorder program")
set(CPACK_COMPONENT_LOGGER_DEPENDS Base GPS NMEA MP I2C)

if (CODE_COVERAGE)
	target_code_coverage(Logger)
	target_code_coverage(PowerHatRead)
endif()