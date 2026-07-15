cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(
	Fraus
	LANGUAGES C
	VERSION 0.1.0
)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Set C standard to C23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(FRAUS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${FRAUS_OUTPUT_DIRECTORY}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${FRAUS_OUTPUT_DIRECTORY}>)

find_package(Vulkan REQUIRED)

# Build Fraus
configure_file(fraus/include/fraus/version.h.in include/fraus/version.h @ONLY)

add_library(fraus STATIC
	# Fonts
	fraus/source/fonts/fonts.c
	fraus/source/fonts/reader.c
	# Images
	fraus/source/images/images.c
	fraus/source/images/inflate.c
	# Models
	fraus/source/models/map.c
	fraus/source/models/models.c
	# Vulkan
	fraus/source/vulkan/object.c
	fraus/source/vulkan/spirv.c
	fraus/source/vulkan/vulkan_utils.c
	fraus/source/vulkan/vulkan.c
	# Fraus
	fraus/source/arena.c
	fraus/source/camera.c
	fraus/source/fraus.c
	fraus/source/math.c
	fraus/source/utils.c
	fraus/source/window.c
)

if(WIN32)
	target_compile_definitions(fraus PUBLIC UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS VK_USE_PLATFORM_WIN32_KHR)
endif()
target_compile_definitions(fraus PUBLIC VK_NO_PROTOTYPES)

if(MSVC)
	target_compile_options(fraus PUBLIC /W4 /WX /utf-8)
	target_link_options(fraus PUBLIC $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>)
else()
	target_compile_options(fraus PUBLIC -Wall -Wextra -pedantic -Werror -pedantic-errors)
endif()

target_include_directories(fraus PUBLIC fraus/include ${Vulkan_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/include)

# Demo

# Compile demo
add_executable(FrausDemo demo/demo.c)
target_link_libraries(FrausDemo PRIVATE fraus)

# Compile the shaders
set(FRAUS_SHADERS shader phong text repeat)

set(FRAUS_SHADERS_VERT ${FRAUS_SHADERS})
list(TRANSFORM FRAUS_SHADERS_VERT APPEND _vert.spv)

set(FRAUS_SHADERS_FRAG ${FRAUS_SHADERS})
list(TRANSFORM FRAUS_SHADERS_FRAG APPEND _frag.spv)

set(FRAUS_SHADERS_SPV ${FRAUS_SHADERS_VERT})
list(APPEND FRAUS_SHADERS_SPV ${FRAUS_SHADERS_FRAG})

list(TRANSFORM FRAUS_SHADERS_SPV PREPEND ${FRAUS_OUTPUT_DIRECTORY}/)

add_custom_target(
	FrausShaders
	DEPENDS ${FRAUS_SHADERS_SPV} ${FRAUS_OUTPUT_DIRECTORY}/vk_layer_settings.txt
)
add_dependencies(FrausDemo FrausShaders)

foreach(SHADER ${FRAUS_SHADERS})
	add_custom_command(
		OUTPUT ${FRAUS_OUTPUT_DIRECTORY}/${SHADER}_vert.spv
		DEPENDS ${CMAKE_SOURCE_DIR}/demo/${SHADER}.vert
		COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --target-env spirv1.6 ${CMAKE_SOURCE_DIR}/demo/${SHADER}.vert -o ${FRAUS_OUTPUT_DIRECTORY}/${SHADER}_vert.spv
	)
	add_custom_command(
		OUTPUT ${FRAUS_OUTPUT_DIRECTORY}/${SHADER}_frag.spv
		DEPENDS ${CMAKE_SOURCE_DIR}/demo/${SHADER}.frag
		COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V --target-env spirv1.6 ${CMAKE_SOURCE_DIR}/demo/${SHADER}.frag -o ${FRAUS_OUTPUT_DIRECTORY}/${SHADER}_frag.spv
	)
endforeach()

# Copy the layers settings
add_custom_command(
	OUTPUT ${FRAUS_OUTPUT_DIRECTORY}/vk_layer_settings.txt
	DEPENDS ${CMAKE_SOURCE_DIR}/demo/vk_layer_settings.txt
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/demo/vk_layer_settings.txt ${FRAUS_OUTPUT_DIRECTORY}/vk_layer_settings.txt
)

# Copy the assets
add_custom_command(
	TARGET FrausDemo
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/demo/assets ${FRAUS_OUTPUT_DIRECTORY}/assets
)

# Tests
enable_testing()
add_executable(FrausTests tests/tests.c)
target_link_libraries(FrausTests PRIVATE fraus)
add_test(
	NAME FrausTests
	COMMAND $<TARGET_FILE:FrausTests>
	WORKING_DIRECTORY ${FRAUS_OUTPUT_DIRECTORY}
)
