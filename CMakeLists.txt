#Vk_SR_Engine\
cmake_minimum_required(VERSION 4.0)

project("VkSREngine")

# Find vulkan
find_package(Vulkan REQUIRED)

add_subdirectory(third_party)

# Set output directories
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

add_subdirectory(src)

# VMA-HPP
target_include_directories(${PROJECT_NAME} PUBLIC 
	"${PROJECT_SOURCE_DIR}/third_party/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator/include"
	)

add_subdirectory(third_party/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator)
add_subdirectory(third_party/VulkanMemoryAllocator-Hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)


# Shader compilation
file(GLOB_RECURSE GLSL_SOURCE_FILES
	"${PROJECT_SOURCE_DIR}/shaders/*.frag"	# Fragment shaders
	"${PROJECT_SOURCE_DIR}/shaders/*.vert"	# Vertex   shaders
	"${PROJECT_SOURCE_DIR}/shaders/*.comp"	# Compute  shaders
	)

foreach(GLSL ${GLSL_SOURCE_FILES})
	message(STATUS "BUILDING SHADER")
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
	message(STATUS ${GLSL})
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
		)
	list(APPEND SPIRV_BINARY_FILES ${spirv})
endforeach()

add_custom_target(shaders
	DEPENDS ${SPIRV_BINARY_FILES}
	)