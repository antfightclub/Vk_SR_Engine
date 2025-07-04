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

# Had to do VMA-HPP in the root CMakeLists.

add_subdirectory(glm)

# STB (for stb_image)
add_library(stb_image INTERFACE)

target_include_directories(stb_image INTERFACE stb) 

# fastgltf
add_subdirectory(fastgltf)

# dear imgui
add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE
	imgui/imgui.h
	imgui/imgui.cpp
	
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_tables.cpp
	imgui/backends/imgui_impl_vulkan.cpp
	imgui/backends/imgui_impl_sdl3.cpp
	)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)
	
