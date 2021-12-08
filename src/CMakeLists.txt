set(DEFAULT_MON_PREFIX "mon" CACHE STRING "Define the default prefix used for the logger output files")
set(DEFAULT_STATE_NAME "SLogger.state" CACHE STRING "Define the default state file name")
set(DEFAULT_MARK_FREQUENCY 10 CACHE STRING "Default local timestamp frequency")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Logger.h.in" "${CMAKE_CURRENT_BINARY_DIR}/Logger.h" @ONLY)

include(GNUInstallDirs)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} PRIVATE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE)

add_executable(Logger Logger.h Logger.c LoggerConfig.c LoggerDMap.c LoggerGPS.c LoggerSignals.c LoggerMP.c LoggerNet.c LoggerNMEA.c LoggerI2C.c LoggerSerial.c LoggerTime.c)
target_link_libraries(Logger PUBLIC Threads::Threads)
target_link_libraries(Logger PUBLIC SELKIELoggerGPS SELKIELoggerBase SELKIELoggerMP SELKIELoggerNMEA SELKIELoggerI2C)
target_link_libraries(Logger PUBLIC inih)

install(TARGETS Logger RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Logger)

set(CPACK_COMPONENT_LOGGER_GROUP "Applications")
set(CPACK_COMPONENT_LOGGER_DESCRIPTION "Main data recorder program")
set(CPACK_COMPONENT_LOGGER_DEPENDS Base GPS NMEA MP I2C)

add_executable(PowerHatRead PowerHatRead.c)
target_link_libraries(PowerHatRead PUBLIC SELKIELoggerI2C)
install(TARGETS PowerHatRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT PowerHat)

set(CPACK_COMPONENT_POWERHAT_GROUP "Applications")
set(CPACK_COMPONENT_POWERHAT_DESCRIPTION "Read values from an INA219 based ADC Hat")

add_executable(AutomationHatRead AutomationHatRead.c)
target_link_libraries(AutomationHatRead PUBLIC SELKIELoggerI2C)
install(TARGETS AutomationHatRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT AutomationHat)

add_executable(AutomationHatLEDTest AutomationHatLEDTest.c)
target_link_libraries(AutomationHatLEDTest PUBLIC SELKIELoggerI2C)
install(TARGETS AutomationHatLEDTest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT AutomationHat)

set(CPACK_COMPONENT_AUTOMATIONHAT_GROUP "Applications")
set(CPACK_COMPONENT_AUTOMATIONHAT_DESCRIPTION "Read values from and control a Pimoroni Automation Hat")

find_package(ZLIB REQUIRED)
add_executable(dat2csv dat2csv.c)
target_link_libraries(dat2csv PUBLIC ZLIB::ZLIB)
target_link_libraries(dat2csv PUBLIC SELKIELoggerBase SELKIELoggerMP)
target_compile_options(dat2csv PRIVATE -Wno-format) # Silence warnings about positional printf arguments
install(TARGETS dat2csv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

find_package(ZLIB REQUIRED)
add_executable(ExtractSatInfo ExtractSatInfo.c)
target_link_libraries(ExtractSatInfo PUBLIC ZLIB::ZLIB)
target_link_libraries(ExtractSatInfo PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS ExtractSatInfo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(ExtractSource ExtractSource.c)
target_link_libraries(ExtractSource PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS ExtractSource RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(DWRead DWRead.c)
target_link_libraries(DWRead PUBLIC SELKIELoggerBase SELKIELoggerDW)
install(TARGETS DWRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion) # In the absence of a better group

set(CPACK_COMPONENT_CONVERSION_GROUP "Applications")
set(CPACK_COMPONONT_CONVERSION_DESCRIPTION "Data format conversion utilities")

if (CODE_COVERAGE)
	target_code_coverage(Logger)
	target_code_coverage(PowerHatRead)
endif()
