option(ENABLE_VALGRIND_CTEST "Add Valgrind support for CTest" ON)
if (ENABLE_VALGRIND_CTEST)
	find_program(MEMORYCHECK_COMMAND valgrind)
	set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --track-origins=yes --trace-children=yes --error-exitcode=66")
endif()

function(memchecked testname command)
	add_test(NAME ${testname} COMMAND ${command} ${ARGN})
	if (ENABLE_VALGRIND_CTEST)
		set(vgcomm "${MEMORYCHECK_COMMAND} ${MEMORYCHECK_COMMAND_OPTIONS} ./${command} ${ARGN}")
		separate_arguments(vgcomm)
		add_test(NAME MemCheck:${testname} COMMAND ${vgcomm})
	endif()
endfunction(memchecked)

add_executable(UBXChecksumTest UBXChecksumTest.c)
target_link_libraries(UBXChecksumTest PUBLIC SELKIELoggerGPS)
memchecked(UBXChecksumTest UBXChecksumTest)

add_executable(UBXMessagesFromFile UBXMessagesFromFile.c)
target_link_libraries(UBXMessagesFromFile PUBLIC SELKIELoggerGPS)
file(COPY testSample.dat DESTINATION .)
memchecked(UBXMessagesFromFile UBXMessagesFromFile testSample.dat)
set_property(TEST UBXMessagesFromFile PROPERTY PASS_REGULAR_EXPRESSION "3 messages read")

add_executable(MPMessagesFromFile MPMessagesFromFile.c)
target_link_libraries(MPMessagesFromFile PUBLIC SELKIELoggerMP)
file(COPY mpTestSample.dat DESTINATION .)
memchecked(MPMessagesFromFile MPMessagesFromFile mpTestSample.dat)
set_property(TEST MPMessagesFromFile PROPERTY PASS_REGULAR_EXPRESSION "90 messages read")

add_executable(QueueTest QueueTest.c)
target_link_libraries(QueueTest PUBLIC SELKIELoggerBase)
add_test(NAME QueueTest COMMAND QueueTest)

add_executable(SATests SATests.c)
target_link_libraries(SATests PUBLIC SELKIELoggerBase)
add_test(NAME SATests COMMAND SATests)

add_executable(MPTests MPTests.c)
target_link_libraries(MPTests PUBLIC SELKIELoggerBase SELKIELoggerMP)
memchecked(MPTests MPTests)
add_test(NAME MPTestsOutput COMMAND bash -c "$<TARGET_FILE:MPTests>|md5sum")
set_property(TEST MPTestsOutput PROPERTY PASS_REGULAR_EXPRESSION "ee530c7e4b7a2c8f16524c0bf1e274cd")

add_executable(NMEAChecksumTest NMEAChecksumTest.c)
target_link_libraries(NMEAChecksumTest PUBLIC SELKIELoggerNMEA)
memchecked(NMEAChecksumTest NMEAChecksumTest)

add_executable(NMEAMessagesFromFile NMEAMessagesFromFile.c)
target_link_libraries(NMEAMessagesFromFile PUBLIC SELKIELoggerNMEA)
file(COPY NMEASample.dat DESTINATION .)
memchecked(NMEAMessagesFromFile NMEAMessagesFromFile NMEASample.dat)
# This tests three lines of output, in order. Note the \n and use of APPEND_STRING to build a single regex
set_property(TEST NMEAMessagesFromFile APPEND_STRING PROPERTY PASS_REGULAR_EXPRESSION "Thu Oct  8 16:15:07 2020\n")
set_property(TEST NMEAMessagesFromFile APPEND_STRING PROPERTY PASS_REGULAR_EXPRESSION "Thu Oct  8 16:15:27 2020\n")
set_property(TEST NMEAMessagesFromFile APPEND_STRING PROPERTY PASS_REGULAR_EXPRESSION "100 messages read")

