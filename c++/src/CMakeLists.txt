cmake_minimum_required(VERSION 3.6.3)

set (CMAKE_CXX_STANDARD 14)

set(SOURCES 
	main.cpp
	draw.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-master/imgui.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-master/imgui_demo.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-master/imgui_draw.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui_impl_glfw_gl3.cpp
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/libs/gl3w/GL/gl3w.c
	${CMAKE_SOURCE_DIR}/3rdparty/lernOpenGL/lib/targa.cpp
	)

add_executable(eduRen ${SOURCES})

include_directories(
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui-master/
	${CMAKE_SOURCE_DIR}/3rdparty/imgui/libs/gl3w/
	${CMAKE_SOURCE_DIR}/3rdparty/lernOpenGL/include/
	${CMAKE_SOURCE_DIR}/3rdparty/stb_image/include/
	)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
target_include_directories(eduRen PUBLIC ${GLFW3_INCLUDE_DIRS})

find_package(OpenGL)

target_link_libraries(eduRen ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES} dl assimp)


#find_library(SERVER_LINK_LIB server ${CMAKE_SOURCE_DIR}/server/lib/)
#include_directories(${CMAKE_SOURCE_DIR}/server/lib/include/)
#include_directories(${CMAKE_SOURCE_DIR}/protocol/include/)
