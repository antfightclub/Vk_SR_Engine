#Vk_SR_Engine\third_party\

## Vulkan (several third party libs will be linking against it)
find_package(Vulkan REQUIRED)

## vkbootstrap
file(GLOB VKBOOTSTRAP_SOURCES "vk-bootstrap/src/*.h" "vk-bootstrap/src/*.cpp")
add_library(vkbootstrap STATIC ${VKBOOTSTRAP_SOURCES})
target_include_directories(vkbootstrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vk-bootstrap/src)

## SDL3
add_subdirectory(SDL)

add_library(sdl INTERFACE)
target_link_libraries(sdl INTERFACE SDL3::SDL3)


