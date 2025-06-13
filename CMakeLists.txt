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

#add_subdirectory(third_party/VulkanMemoryAllocator-Hpp/Vulkan-Headers)
add_subdirectory(third_party/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator)
add_subdirectory(third_party/VulkanMemoryAllocator-Hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
#target_include_directories(${PROJECT_NAME} PUBLIC 
#	${PROJECT_SOURCE_DIR}/third_party/VulkanMemoryAllocator-Hpp/Vulkan-Headers 
#	${PROJECT_SOURCE_DIR}/third_party/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator/include
#	)