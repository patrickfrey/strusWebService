cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( CppCmsHelloWorld ${CMAKE_CURRENT_BINARY_DIR}/src/testCppCmsHelloWorld )

set_tests_properties( CppCmsHelloWorld PROPERTIES ENVIRONMENT LD_LIBRARY_PATH=${LIBBOOSTER_LIBRARY_DIR} )


