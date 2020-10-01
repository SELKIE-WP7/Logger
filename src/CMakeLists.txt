set(DEFAULT_GPS_PORT "/dev/ttyUSB0" CACHE STRING "Define the default port used to communicate with the GPS module")
set(DEFAULT_MON_PREFIX "mon" CACHE STRING "Define the default prefix used for the logger output files")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Logger.h.in" "${CMAKE_CURRENT_BINARY_DIR}/Logger.h" @ONLY)

find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR} PRIVATE)
add_executable(Logger Logger.h Logger.c)
target_link_libraries(Logger PUBLIC ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(Logger PUBLIC SELKIELoggerGPS SELKIELoggerBase)

