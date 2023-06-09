list(APPEND SL_Base_SRC logging.c messages.c queue.c serial.c strarray.c)
list(APPEND SL_Base_INC logging.h messages.h queue.h serial.h sources.h strarray.h)

find_package(Threads REQUIRED)

add_library(SELKIELoggerBase ${SL_Base_SRC})
target_link_libraries(SELKIELoggerBase PUBLIC ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(SELKIELoggerBase PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(SELKIELoggerBase PROPERTIES SONAME 1)

set_target_properties(SELKIELoggerBase PROPERTIES PUBLIC_HEADER SELKIELoggerBase.h)
set_target_properties(SELKIELoggerBase PROPERTIES PRIVATE_HEADER "${SL_Base_INC}")

include(GNUInstallDirs)

target_include_directories(SELKIELoggerBase PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)
target_include_directories(SELKIELoggerBase PRIVATE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/library/>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger>)

install(TARGETS SELKIELoggerBase
	EXPORT SLPubLibs
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Base
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger COMPONENT BaseDev
	PRIVATE_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SELKIELogger/base COMPONENT BaseDev
	)

set(CPACK_COMPONENT_BASE_GROUP lib)
set(CPACK_COMPONENT_BASE_DISPLAY_NAME "SELKIE Base Library")
set(CPACK_COMPONENT_BASEDEV_GROUP dev)
set(CPACK_COMPONENT_BASEDEV_DISPLAY_NAME "SELKIE Base Library - Headers")
set(CPACK_COMPONENT_BASEDEV_DEPENDS Base)
