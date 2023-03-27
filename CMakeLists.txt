cmake_minimum_required(VERSION 3.0)

project(GenshinAutoPickup VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)
add_compile_options(/MT /GR- /utf-8)

include_directories(
    modules
    third_party
    third_party/opencv/include)

set(opencv ${CMAKE_SOURCE_DIR}/third_party/opencv/opencv_world470.lib)

add_executable(test src/test/main.cc)
target_link_libraries(test d3d11.lib ${opencv})

add_executable(preprocess src/preprocess/main.cc)
target_link_libraries(preprocess ${opencv})

add_executable(${PROJECT_NAME} src/main/main.cc)
target_link_libraries(${PROJECT_NAME} d3d11.lib ${opencv})
add_custom_command(TARGET ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/third_party/opencv/opencv_world470.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>/opencv_world470.dll
    COMMENT "Copy dependencies")
add_custom_command(TARGET ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMENT "Copy resources")