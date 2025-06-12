#Vk_SR_Engine\src\

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
#target_link_libraries(${PROJECT_NAME} PRIVATE third_party)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
#  COMMAND_EXPAND_LISTS
#  )