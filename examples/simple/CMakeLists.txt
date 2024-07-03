include_directories("../../")
add_executable(simple main.cpp)
target_include_directories(simple SYSTEM PRIVATE ../external)
