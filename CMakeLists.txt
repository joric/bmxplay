cmake_minimum_required(VERSION 2.6)

project(bmxplay)

set(root ${CMAKE_CURRENT_SOURCE_DIR})

#include_directories(${root})

macro(add_machine name type)
  add_library(${name} SHARED src/${name}.cpp src/${name}.h)
  list(APPEND ${type} ${name})
endmacro()

macro(add_generator name)
  add_machine(${name} generators)
endmacro()

macro(add_effect name)
  add_machine(${name} effects)
endmacro()

add_executable(bmxplay src/bmxplay.c)
add_executable(bmxplayer src/bmxplayer.cpp)

add_generator(_303)
add_generator(_xi)
add_generator(_voice)
add_effect(_filter)
add_effect(_delay)

if (WIN32)
  target_link_libraries(bmxplay winmm)
  target_link_libraries(bmxplayer winmm)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif(WIN32)

#set(buzz ${root}/bin)
set(buzz D:/Projects/home/music/Buzz)

install(TARGETS ${generators} RUNTIME DESTINATION ${buzz}/gear/generators)
install(TARGETS ${effects} RUNTIME DESTINATION ${buzz}/gear/effects)
install(TARGETS bmxplay bmxplayer RUNTIME DESTINATION ${root}/bin)
