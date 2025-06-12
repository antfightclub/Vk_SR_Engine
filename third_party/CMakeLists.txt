#Vk_SR_Engine\third_party\
add_subdirectory(SDL)

add_library(sdl INTERFACE)
target_link_libraries(sdl INTERFACE SDL3::SDL3)

