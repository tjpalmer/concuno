add_executable(
  concuno-test
  concuno-test.c
)

target_link_libraries(
  concuno-test
  concuno-static
  ${math_LIBRARY}
)
