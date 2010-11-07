add_executable(
  stackiter-learner
  stackiter-learner.cpp
  loader.cpp
  state.cpp
)

target_link_libraries(
  stackiter-learner
  cuncuno
)
