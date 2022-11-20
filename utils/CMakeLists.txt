include(GNUInstallDirs)

include_directories(${CMAKE_CURRENT_BINARY_DIR} PRIVATE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE)

add_executable(PowerHatRead PowerHatRead.c)
target_link_libraries(PowerHatRead PUBLIC SELKIELoggerI2C)
target_link_libraries(PowerHatRead PUBLIC m)
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
target_compile_options(dat2csv PRIVATE -Wno-format -Wno-format-security) # Silence warnings about positional printf arguments
install(TARGETS dat2csv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(DumpMessages DumpMessages.c)
target_link_libraries(DumpMessages PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS DumpMessages RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

find_package(ZLIB REQUIRED)
add_executable(ExtractSatInfo ExtractSatInfo.c)
target_link_libraries(ExtractSatInfo PUBLIC ZLIB::ZLIB)
target_link_libraries(ExtractSatInfo PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS ExtractSatInfo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(ExtractSource ExtractSource.c)
target_link_libraries(ExtractSource PUBLIC SELKIELoggerBase SELKIELoggerMP)
install(TARGETS ExtractSource RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)
if (CODE_COVERAGE)
	target_code_coverage(ExtractSource AUTO ALL ARGS -r -S 0x30 -f -o ${PROJECT_BINARY_DIR}/tests/mpTestSampleExtracted.dat  ${PROJECT_SOURCE_DIR}/tests/mpTestSample.dat  COVERAGE_TARGET_NAME ExtractSourceTest)
endif()

add_executable(DWRead DWRead.c)
target_link_libraries(DWRead PUBLIC SELKIELoggerBase SELKIELoggerDW)
install(TARGETS DWRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion) # In the absence of a better group
if (CODE_COVERAGE)
	target_code_coverage(DWRead AUTO ALL ARGS -x ${PROJECT_SOURCE_DIR}/tests/DWTestSample.hxv COVERAGE_TARGET_NAME DWReadTest)
endif()

add_executable(MQTTTest MQTTTest.c)
target_link_libraries(MQTTTest PUBLIC SELKIELoggerBase SELKIELoggerMQTT)
install(TARGETS MQTTTest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(N2KRead N2KRead.c)
target_link_libraries(N2KRead PUBLIC SELKIELoggerBase SELKIELoggerN2K)
install(TARGETS N2KRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(N2KClassify N2KClassify.c)
target_link_libraries(N2KClassify PUBLIC SELKIELoggerBase SELKIELoggerN2K)
install(TARGETS N2KClassify RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(LPMSRead LPMSRead.c)
target_link_libraries(LPMSRead PUBLIC SELKIELoggerBase SELKIELoggerLPMS)
install(TARGETS LPMSRead RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

add_executable(LPMSTest LPMSTest.c)
target_link_libraries(LPMSTest PUBLIC SELKIELoggerBase SELKIELoggerLPMS)
install(TARGETS LPMSTest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Conversion)

set(CPACK_COMPONENT_CONVERSION_GROUP "Applications")
set(CPACK_COMPONONT_CONVERSION_DESCRIPTION "Data format conversion/inspection utilities")

if (CODE_COVERAGE)
	target_code_coverage(AutomationHatRead)
	target_code_coverage(AutomationHatLEDTest)
	target_code_coverage(dat2csv)
	target_code_coverage(ExtractSatInfo)
	target_code_coverage(ExtractSource)
	target_code_coverage(DWRead)
	target_code_coverage(MQTTTest)
	target_code_coverage(PowerHatRead)
endif()
