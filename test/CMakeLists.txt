find_library(CPPSPEC_LIBRARY NAMES CppSpec)

add_executable(yamlppspecs main.cpp)
target_link_libraries(yamlppspecs ${CPPSPEC_LIBRARY})