add_executable(UBXChecksumTest UBXChecksumTest.c)
target_link_libraries(UBXChecksumTest PUBLIC SELKIELoggerGPS)
add_test(NAME UBXChecksumTest COMMAND UBXChecksumTest)

add_executable(UBXMessagesFromFile UBXMessagesFromFile.c)
target_link_libraries(UBXMessagesFromFile PUBLIC SELKIELoggerGPS)
file(COPY testSample.dat DESTINATION .)
add_test(NAME UBXMessagesFromFile COMMAND UBXMessagesFromFile testSample.dat)
set_property(TEST UBXMessagesFromFile PROPERTY PASS_REGULAR_EXPRESSION "3 messages read")

add_executable(MPMessagesFromFile MPMessagesFromFile.c)
target_link_libraries(MPMessagesFromFile PUBLIC SELKIELoggerMP)
file(COPY testSample.dat DESTINATION .)
add_test(NAME MPMessagesFromFile COMMAND MPMessagesFromFile testSample.dat)

add_executable(QueueTest QueueTest.c)
target_link_libraries(QueueTest PUBLIC SELKIELoggerBase)
add_test(NAME QueueTest COMMAND QueueTest)

add_executable(SATests SATests.c)
target_link_libraries(SATests PUBLIC SELKIELoggerBase)
add_test(NAME SATests COMMAND SATests)

add_executable(MPTests MPTests.c)
target_link_libraries(MPTests PUBLIC SELKIELoggerBase SELKIELoggerMP)
add_test(NAME MPTests COMMAND MPTests)
add_test(NAME MPTestsOutput COMMAND bash -c "$<TARGET_FILE:MPTests>|md5sum")
set_property(TEST MPTestsOutput PROPERTY PASS_REGULAR_EXPRESSION "48ece0e95633a32450a4aa98939968e6")
