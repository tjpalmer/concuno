add_executable(
  concuno-run
  run.cpp
  args.cpp
  data.cpp
)

target_link_libraries(
  concuno-run
  concuno-static
  ${math_LIBRARY}
)
