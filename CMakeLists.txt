cmake_minimum_required(VERSION 2.4)

project(cuncuno)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug
    CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE
  )
endif(NOT CMAKE_BUILD_TYPE)

set(cuncuno_INCLUDE_DIR ${cuncuno_SOURCE_DIR}/include)

include_directories(
  ${cuncuno_INCLUDE_DIR}
)

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS "-Wall")
  # set(CMAKE_CXX_FLAGS "-pg") # for profiling -- tie to debug, too?
endif(CMAKE_COMPILER_IS_GNUCC)

subdirs(
  src
)
