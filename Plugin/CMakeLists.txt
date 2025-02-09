cmake_minimum_required(VERSION 3.21)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# info
project(
	NativeAnimationFrameworkSF
	VERSION 1.2.3
	LANGUAGES CXX
)

# boiler
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

# in-place configuration
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Plugin.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

# source files
execute_process(COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/!update.ps1" "SOURCEGEN" "${PROJECT_VERSION}" "${CMAKE_CURRENT_BINARY_DIR}")
include(${CMAKE_CURRENT_BINARY_DIR}/sourcelist.cmake)
source_group(
	TREE ${CMAKE_CURRENT_SOURCE_DIR}
	FILES ${SOURCES}
)

source_group(
	TREE ${CMAKE_CURRENT_BINARY_DIR}
	FILES ${CMAKE_CURRENT_BINARY_DIR}/include/Plugin.h
)

# dependencies
find_package(spdlog CONFIG REQUIRED)
add_subdirectory("../extern/NAF-Common" NAF-Common)

file(GLOB_RECURSE SRC_FILES2 src/*.cpp)

# cmake target
# runtime
add_library(
	${PROJECT_NAME}
	SHARED
		${CMAKE_CURRENT_BINARY_DIR}/include/Plugin.h
		${CMAKE_CURRENT_BINARY_DIR}/version.rc
		.clang-format
		vcpkg.json
		${SRC_FILES2})

# copy to Starfield directory
set(STARFIELD_PATH "C:/Program Files (x86)/Steam/steamapps/common/Starfield/Data/SFSE/Plugins")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${STARFIELD_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${STARFIELD_PATH}
    COMMENT "Copying plugin to Starfield directory..."
)

# include dir
target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

# linkage
target_link_libraries(
	${PROJECT_NAME} 
	PRIVATE
		spdlog::spdlog
		NAF-Common
)

# compiler def
if (MSVC)
	add_compile_definitions(_UNICODE)

	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
			/MP
			/await
			/W0
			/WX
			/permissive-
			/utf-8
			/Zc:alignedNew
			/Zc:auto
			/Zc:__cplusplus
			/Zc:externC
			/Zc:externConstexpr
			/Zc:forScope
			/Zc:hiddenFriend
			/Zc:implicitNoexcept
			/Zc:lambda
			/Zc:noexceptTypes
			/Zc:preprocessor
			/Zc:referenceBinding
			/Zc:rvalueCast
			/Zc:sizedDealloc
			/Zc:strictStrings
			/Zc:ternary
			/Zc:threadSafeInit
			/Zc:trigraphs
			/Zc:wchar_t
			/wd4200 # nonstandard extension used : zero-sized array in struct/union
			/FI${CMAKE_CURRENT_SOURCE_DIR}/src/PCH.h
	)
endif()

# PCH
target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
		src/PCH.h
)

set_property(
	TARGET 
	${PROJECT_NAME}
	PROPERTY VS_USER_PROPS 
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stl_modules.props"
)
