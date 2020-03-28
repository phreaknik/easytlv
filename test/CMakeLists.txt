cmake_minimum_required(VERSION 3.10)

# set the project name
project(etlv_test)

# add the executable
add_executable(etlv_test test.c ../easytlv.c)

# compile-time defines
#target_compile_definitions(etlv_test PRIVATE ETLV_DEBUG)

# includes
target_include_directories( etlv_test PUBLIC
                            "${PROJECT_BINARY_DIR}"
                            "./"
                            "../"
                            )

