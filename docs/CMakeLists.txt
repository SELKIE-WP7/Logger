option(BUILD_DOCUMENTATION "Build developer documentation with Doxygen" ON)
if (BUILD_DOCUMENTATION)
	find_package(Doxygen REQUIRED dot)
	set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/doxygen/")
	set(DOXYGEN_FILE_PATTERNS "*.c,*.h,*.in,*.md,*.py")
	set(DOXYGEN_EXTENSION_MAPPING "in=c++")
	set(DOXYGEN_EXTRACT_ALL NO)
	set(DOXYGEN_EXTRACT_PRIVATE YES)
	set(DOXYGEN_EXTRACT_STATIC YES)
	set(DOXYGEN_SOURCE_BROWSER YES)
	set(DOXYGEN_SOURCE_TOOLTIPS YES)
	set(DOXYGEN_WARN_NO_PARAMDOC YES)
	set(DOXYGEN_EXCLUDE_PATTERNS */build/* */dist/*)
	set(DOXYGEN_QUIET YES)
	set(DOXYGEN_IMAGE_PATH docs/images)
	set(DOXYGEN_MACRO_EXPANSION YES)
	set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
	set(DOXYGEN_PREDEFINED "__attribute__(x)=" "_Static_assert(x,y)=")
	doxygen_add_docs(doxygen ${PROJECT_SOURCE_DIR}/docs/ ${PROJECT_SOURCE_DIR}/logger/ ${PROJECT_SOURCE_DIR}/utils/ ${PROJECT_SOURCE_DIR}/library/ ${PROJECT_SOURCE_DIR}/tests/ ${PROJECT_SOURCE_DIR}/python/ COMMENT "Generate developer documentation")
endif()


function(md2man name section source)
	set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/man/man${section}/${name}.${section}.gz")
	add_custom_command(TARGET manpages PRE_BUILD
		BYPRODUCTS ${OUTFILE}
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build_man_page.sh ${name} ${section} ${source} ${OUTFILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		MAIN_DEPENDENCY ${source} VERBATIM
		)
	install(FILES ${OUTFILE} DESTINATION "${CMAKE_INSTALL_MANDIR}/man${section}" COMPONENT manpages)
endfunction()

option(BUILD_MAN_PAGES "Build (and later, install) man pages" ON)
if (BUILD_MAN_PAGES)
	find_program(PANDOC pandoc DOC "Path to pandoc")
	add_custom_target(manpages ALL COMMENT "Building manual pages")
	set(CPACK_COMPONENT_MANPAGES_DESCRIPTION "Documentation for use with man command")
	md2man("SLExtract" 1 "manual/SLExtract.md")
	md2man("SLClassify" 1 "manual/SLClassify.md")
	md2man("SLConvert" 1 "manual/SLConvert.md")
	md2man("ExtractSatInfo" 1 "manual/ExtractSatInfo.md")
	md2man("ExtractSource" 1 "manual/ExtractSource.md")
endif()
