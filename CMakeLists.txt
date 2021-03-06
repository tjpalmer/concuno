cmake_minimum_required(VERSION 2.4)

project(concuno) # Or benixt?

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug
    CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE
  )
endif(NOT CMAKE_BUILD_TYPE)

set(
  Eigen_INCLUDE_DIR "${concuno_SOURCE_DIR}/../eigen"
  CACHE STRING "The directory for Eigen header files."
)

#set(concuno_INCLUDE_DIR ${concuno_SOURCE_DIR}/include)

# TODO Use Eigen blas?
#find_library(blas_LIBRARY atlas)
#find_library(cblas_LIBRARY cblas)
#find_library(lapack_LIBRARY lapack)
find_library(math_LIBRARY m)

include_directories(
  #${concuno_INCLUDE_DIR}
  #${Eigen_INCLUDE_DIR}
  c
)

if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "-Wall -Wdeclaration-after-statement")
  #set(CMAKE_C_FLAGS_DEBUG "-pg")
  set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
endif(CMAKE_COMPILER_IS_GNUCC)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  # Clang uses C++11 by default, it seems, but warns about potential abuses.
  # Turn off those warnings.
  set(CMAKE_CXX_FLAGS "-Wall -Wno-c++11-extensions")
endif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

subdirs(
  c
  #cpp
)
