add_executable(
  rcss-test
  rcss-test.cpp
  choose.cpp
  domain.cpp
  load.cpp
)

target_link_libraries(
  rcss-test
  concuno-static
  ${math_LIBRARY}
  yajl-static
)
