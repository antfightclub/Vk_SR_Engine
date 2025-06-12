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
