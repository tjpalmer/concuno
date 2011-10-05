add_executable(
  rcss-test
  rcss-test.c
  choose.c
  domain.c
  load.c
)

target_link_libraries(
  rcss-test
  concuno-static
  ${math_LIBRARY}
)
