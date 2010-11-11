add_library(cuncuno SHARED
  entity.cpp
	tree.cpp
)

add_executable(cuno cuno.cpp)

target_link_libraries(cuno cuncuno)
