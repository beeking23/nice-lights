cmake_minimum_required(VERSION 3.5)
project(nice-lights)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../imgui")
include_directories("${IMGUI_DIR}")
include_directories("${IMGUI_DIR}/backends")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../glm")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/e131")

add_definitions(-DGLEW_STATIC=1)
add_compile_options("-std=c++20")

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

# Create your game executable target as usual
add_executable(nice-lights
  src/main.cpp
  src/icosahedron.cpp 
  src/icosahedron.hpp 
  src/vertexdesc.hpp 
  src/vertexdesc.cpp 
  src/glhelpers.hpp 
  src/glhelpers.cpp 
  e131/e131.c 
  src/network.cpp 
  src/network.hpp 
  src/serial.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp )
 

# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
  # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
  target_link_libraries(nice-lights PRIVATE SDL2::SDL2main)
endif()

if(WIN32)
  target_link_libraries(nice-lights PRIVATE SDL2::SDL2 ws2_32 glew32 opengl32 user32 gdi32 kernel32)
else()
  target_link_libraries(nice-lights PRIVATE SDL2::SDL2 GL GLEW)
endif()


