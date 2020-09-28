add_executable(UBXChecksumTest UBXChecksumTest.c)
target_link_libraries(UBXChecksumTest PUBLIC SELKIELoggerGPS)
add_test(NAME UBXChecksumTests COMMAND UBXChecksumTest)

add_executable(UBXMessagesFromFile UBXMessagesFromFile.c)
target_link_libraries(UBXMessagesFromFile PUBLIC SELKIELoggerGPS)
file(COPY testSample.dat DESTINATION .)
add_test(NAME UBXMessagesFromFile COMMAND UBXMessagesFromFile testSample.dat)

add_executable(QueueTest QueueTest.c)
target_link_libraries(QueueTest PUBLIC SELKIELoggerBase)
add_test(NAME QueueTest COMMAND QueueTest)
