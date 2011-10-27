add_executable(
  concuno-test
  concuno-test.cpp
)

target_link_libraries(
  concuno-test
  concuno-static
  ${math_LIBRARY}
)
