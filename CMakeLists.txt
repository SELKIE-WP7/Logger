cmake_minimum_required(VERSION 3.12...3.17)

project(SELKIELogger LANGUAGES C)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
add_compile_options(--std=c99 -Wall -pedantic -Werror -g -O2)
add_compile_definitions(_GNU_SOURCE)
add_compile_definitions(_FORTIFY_SOURCE=2)

find_package(Git)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --dirty=X --tags OUTPUT_VARIABLE GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND ${GIT_EXECUTABLE} symbolic-ref -q --short HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(REGEX MATCH "^v([0-9]+)\.([0-9]+)-([0-9]+)-(.*)$" RM ${GIT_VERSION})
	project(${PROJECT_NAME} VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})

	string(COMPARE EQUAL ${GIT_BRANCH} "trunk" ON_TRUNK)
	if (${ON_TRUNK})
		string(CONCAT ver ${GIT_VERSION})
	else()
		string(CONCAT ver ${GIT_VERSION} "-" ${GIT_BRANCH})
	endif()
	set(VERSION_STRING ${ver} CACHE STRING "Project version information" FORCE)

else()
	message(WARNING "Unable to extract version from git")
	set(VERSION_STRING "unversioned" CACHE STRING "Project version information" FORCE)
endif()

include(GNUInstallDirs)

include_directories(${CMAKE_BINARY_DIR})

option(USE_STABLECODER_SCRIPTS "Use CMake scripts from https://github.com/StableCoder/cmake-scripts to provide convenient features" OFF)
if (USE_STABLECODER_SCRIPTS)
	set(STABLECODER_PATH "../cmake-scripts/" CACHE PATH "Path to StableCoder CMake scripts")
	set(CMAKE_MODULE_PATH APPEND "${STABLECODER_PATH}")
	include(code-coverage)
	add_code_coverage()
	add_code_coverage_all_targets()

	include(sanitizers)
endif()

add_subdirectory(library)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

option(BUILD_DOCUMENTATION "Build developer documentation with Doxygen" ON)
if (BUILD_DOCUMENTATION)
	find_package(Doxygen REQUIRED dot)
	set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/")
	set(DOXYGEN_EXCLUDE build/)
	set(DOXYGEN_FILE_PATTERNS "*.c,*.h,*.in,*.md")
	set(DOXYGEN_EXTENSION_MAPPING "in=c++")
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_EXTRACT_PRIVATE YES)
	set(DOXYGEN_EXTRACT_STATIC YES)
	set(DOXYGEN_SOURCE_BROWSER YES)
	set(DOXYGEN_SOURCE_TOOLTIPS YES)
	set(DOXYGEN_QUIET YES)
	set(DOXYGEN_MACRO_EXPANSION YES)
	set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
	set(DOXYGEN_PREDEFINED "__attribute__(x)=")
	doxygen_add_docs(doxygen ALL ${PROJECT_SOURCE_DIR} COMMENT "Generate developer documentation")
endif()

#find_library(MATH_LIBRARY m)



set(CPACK_PACKAGE_VENDOR "Swansea University")
set(CPACK_PACKAGE_CONTACT "t.lake@swansea.ac.uk")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SELKIE Datalogger core software")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "RPM;DEB;TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    /.git
    /dist
    /.*build.*
    /\\\\.DS_Store
)

include(CPack)
