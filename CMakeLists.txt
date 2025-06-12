#Vk_SR_Engine\
cmake_minimum_required(VERSION 4.0)

project(Vk_SR_Engine)

# Find vulkan
find_package(Vulkan REQUIRED)



add_subdirectory(third_party)
add_subdirectory(src)
