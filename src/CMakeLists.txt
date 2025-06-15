#Vk_SR_Engine\src\
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE 
	main.cpp
	vk_engine.h
	vk_engine.cpp
	vk_types.h
	vk_initializers.h
	vk_initializers.cpp
	vk_descriptors.h
	vk_descriptors.cpp
	vk_pipelines.h
	vk_pipelines.cpp
	vk_images.h
	vk_images.cpp
	compute_structs.h
	)

set_property (TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_include_directories (${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


# SDL
target_link_libraries(${PROJECT_NAME} PRIVATE sdl)

# main
target_link_libraries (${PROJECT_NAME} PUBLIC
	Vulkan::Vulkan
	fmt::fmt
	vkbootstrap
	glm::glm
	)

target_precompile_headers (${PROJECT_NAME} PUBLIC
	<vulkan/vulkan.hpp>
	)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS
  )