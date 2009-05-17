find_library(CPPSPEC_LIBRARY NAMES CppSpec)

set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} " -Wall -g")
add_executable(yamlppspecs main.cpp)
target_link_libraries(yamlppspecs ${CPPSPEC_LIBRARY})