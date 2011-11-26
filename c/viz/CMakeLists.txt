# TODO Presumably other scripts will go here.

# viz-scan-points
add_executable(viz-scan-points viz-scan-points.cpp)
target_link_libraries(viz-scan-points concuno-static ${math_LIBRARY})
