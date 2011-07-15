add_executable(
  vision-learn
  vision-learn.c
)

target_link_libraries(
  vision-learn
  concuno
  ${math_LIBRARY}
)
