cmake_minimum_required(VERSION 3.16)

project(FlappyBird LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(SFML_STATIC_LIBRARIES TRUE)

set(RAYLIB_PATH ${CMAKE_SOURCE_DIR}/vendor/raylib)
set(RAYLIB_LIBRARY ${RAYLIB_PATH}/lib/libraylib.a)

include_directories(${CMAKE_SOURCE_DIR}/vendor/raylib/include)

add_executable(${PROJECT_NAME}
	src/main.cpp
)

target_link_libraries(${PROJECT_NAME} ${RAYLIB_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR} )
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR} )

if (APPLE)
	target_link_libraries(${PROJECT_NAME} "-framework IOKit")
	target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
	target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
