cmake_minimum_required(VERSION 3.6.3)

set (CMAKE_CXX_STANDARD 14)


set(SOURCES 
	main.cpp
	draw.cpp
	imgui/imgui-master/imgui.cpp
	imgui/imgui-master/imgui_demo.cpp
	imgui/imgui-master/imgui_draw.cpp
	imgui/imgui_impl_glfw_gl3.cpp
	imgui/libs/gl3w/GL/gl3w.c
	${CMAKE_SOURCE_DIR}/lib/LoadShaders.cpp
	${CMAKE_SOURCE_DIR}/lib/targa.cpp
	)

add_executable(eduRen ${SOURCES})

include_directories(
	imgui/
	imgui/imgui-master/
 	imgui/imgui-master/examples/ 
	imgui/libs/gl3w/
	${CMAKE_SOURCE_DIR}/include/
	)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
target_include_directories(eduRen PUBLIC ${GLFW3_INCLUDE_DIRS})

find_package(OpenGL)

#find_library(SERVER_LINK_LIB server ${CMAKE_SOURCE_DIR}/server/lib/)
#include_directories(${CMAKE_SOURCE_DIR}/server/lib/include/)
#include_directories(${CMAKE_SOURCE_DIR}/protocol/include/)

#find_library(PROTOCOL_LINK_LIB Protocol ${CMAKE_SOURCE_DIR}/protocol/)


target_link_libraries(eduRen ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES} dl)

#findnk_libraries(testapp ${SDL2_LIBRARIES})
#include_directories(${CMAKE_SOURCE_DIR}/server/)
#message("found lib"  ${SERVER_LINK_LIB})

#target_link_libraries(sv_run ${SERVER_LINK_LIB})
