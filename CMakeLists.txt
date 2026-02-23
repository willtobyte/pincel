cmake_minimum_required(VERSION 3.15)

if(APPLE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.macos.txt)
elseif(WIN32)
  include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.windows.txt)
else()
  message(FATAL_ERROR "Unsupported platform")
endif()
