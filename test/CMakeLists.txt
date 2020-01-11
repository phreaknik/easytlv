cmake_minimum_required(VERSION 3.10)

# set the project name
project(test)

# add the executable
add_executable(test test.c ../easytlv.c)

# compile-time defines
#target_compile_definitions(test PRIVATE ETLV_DEBUG)

# includes
target_include_directories( test PUBLIC
                            "${PROJECT_BINARY_DIR}"
                            "./"
                            "../"
                            )

