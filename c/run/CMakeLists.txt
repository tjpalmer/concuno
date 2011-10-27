add_executable(
  concuno-run
  run.cpp
)

target_link_libraries(
  concuno-run
  concuno-static
  ${math_LIBRARY}
)
