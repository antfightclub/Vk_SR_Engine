#Vk_SR_Engine\third_party\

## Vulkan (several third party libs will be linking against it)
find_package(Vulkan REQUIRED)

## fmt for console printouts
add_subdirectory(fmt EXCLUDE_FROM_ALL)


## vkbootstrap
file(GLOB VKBOOTSTRAP_SOURCES "vk-bootstrap/src/*.h" "vk-bootstrap/src/*.cpp")
add_library(vkbootstrap STATIC ${VKBOOTSTRAP_SOURCES})
target_include_directories(vkbootstrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property (TARGET vkbootstrap PROPERTY CXX_STANDARD 20)

## SDL3
add_subdirectory(SDL)

add_library(sdl INTERFACE)
target_link_libraries(sdl INTERFACE SDL3::SDL3)

## Vulkan Memory Allocator (HPP bindings!)
#add_subdirectory(VulkanMemoryAllocator-Hpp/Vulkan-Headers)
add_subdirectory(VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator)
add_subdirectory(VulkanMemoryAllocator-Hpp)
add_subdirectory(glm)

