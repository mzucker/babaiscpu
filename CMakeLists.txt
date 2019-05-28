cmake_minimum_required(VERSION 2.8)

project(babaiscpu)
set(CMAKE_C_FLAGS "-g -Wall")

if(APPLE)
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
endif(APPLE)

add_executable(babaiscpu babaiscpu.c gamecore.c)
#target_link_libraries(babaiscpu glfw ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${JANSSON_LIBRARIES} ${CURL_LIBRARIES} png jpeg m)
