include_directories(
  # Clients shouldn't dig directly into these, but cuncuno itself can.
  ${cuncuno_SOURCE_DIR}/c
)

add_executable(
  stackiter-learner
  stackiter-learner.c
  #chooser.cpp
  loader.c
  #state.cpp
)

target_link_libraries(
  stackiter-learner
  cuncuno
)
