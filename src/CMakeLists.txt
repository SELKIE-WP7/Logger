set(DEFAULT_MON_PREFIX "mon" CACHE STRING "Define the default prefix used for the logger output files")
set(DEFAULT_STATE_NAME "SLogger.state" CACHE STRING "Define the default state file name")
set(DEFAULT_MARK_FREQUENCY 10 CACHE STRING "Default local timestamp frequency")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Logger.h.in" "${CMAKE_CURRENT_BINARY_DIR}/Logger.h" @ONLY)

include(GNUInstallDirs)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} PRIVATE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE)

add_executable(Logger Logger.h Logger.c LoggerGPS.h LoggerGPS.c LoggerSignals.c LoggerMP.c LoggerNMEA.c LoggerI2C.c LoggerTime.c)
target_link_libraries(Logger PUBLIC Threads::Threads)
target_link_libraries(Logger PUBLIC SELKIELoggerGPS SELKIELoggerBase SELKIELoggerMP SELKIELoggerNMEA SELKIELoggerI2C)

install(TARGETS Logger RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Logger)

set(CPACK_COMPONENT_LOGGER_GROUP "Applications")
set(CPACK_COMPONENT_LOGGER_DESCRIPTION "Main data recorder program")
set(CPACK_COMPONENT_LOGGER_DEPENDS Base GPS NMEA MP I2C)

add_executable(PowerHatRead PowerHatRead.c)
target_link_libraries(PowerHatRead PUBLIC SELKIELoggerI2C)
install(TARGETS PowerHatRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT PowerHat)

set(CPACK_COMPONENT_POWERHAT_GROUP "Applications")
set(CPACK_COMPONENT_POWERHAT_DESCRIPTION "Read values from an INA219 based ADC Hat")

find_package(ZLIB REQUIRED)
add_executable(dat2csv dat2csv.c)
target_link_libraries(dat2csv PUBLIC ZLIB::ZLIB)
target_link_libraries(dat2csv PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS dat2csv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

find_package(ZLIB REQUIRED)
add_executable(ExtractSatInfo ExtractSatInfo.c)
target_link_libraries(ExtractSatInfo PUBLIC ZLIB::ZLIB)
target_link_libraries(ExtractSatInfo PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS ExtractSatInfo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

set(CPACK_COMPONENT_CONVERSION_GROUP "Applications")
set(CPACK_COMPONONT_CONVERSION_DESCRIPTION "Data format conversion utilities")

if (CODE_COVERAGE)
	target_code_coverage(Logger)
	target_code_coverage(PowerHatRead)
endif()
