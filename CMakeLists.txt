cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

execute_process(
  COMMAND git describe --tags --long --always
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE PROJECT_VER
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT PROJECT_VER)
    string(TIMESTAMP TODAY "%Y.%m.%d")
    set(PROJECT_VER "${TODAY}-nogit")
endif()

project(my-project)
