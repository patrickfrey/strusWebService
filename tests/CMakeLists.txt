cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories( "${PROJECT_SOURCE_DIR}/3rdParty/curlcpp/include" "${CURL_INCLUDE_DIRS}" )

add_executable( curlJsonTester curlJsonTester.cpp)
target_link_libraries( curlJsonTester "${CURL_LIBRARIES}" curlpp_static)

add_test( NAME ping COMMAND curlJsonTester ping )

# basically nothing works down here!

# TODO: make a C command doing the starting in the backgroud,
# the curl sending and comapring the result in JSON
#add_test( NAME ping COMMAND ${SHELL} ${CMAKE_CURRENT_SOURCE_DIR}/tests/execute_test.sh ping )

#add_test( NAME version COMMAND curl -v http://localhost:8080/version )

# TODO: how to escape JSON as parameter, cmake is not really helpful here!
#add_test( NAME createA COMMAND curl -v -XPOST -H "Content-Type: application/json" http://localhost:8080/index/create/A -d '{ "params": {} }' )
#add_test( NAME createB COMMAND curl -v -XPOST -H "Content-Type: application/json" http://localhost:8080/index/create/B -d '{ "params": {} }' )

#add_test( NAME statsA COMMAND curl -v http://localhost:8080/index/stats/A )

#add_test( NAME configA COMMAND curl -v http://localhost:8080/index/config/A )

#add_test( NAME list COMMAND curl -v http://localhost:8080/index/list )

#add_test( NAME list COMMAND curl -v http://localhost:8080/document/insert -d '{ "doc" : { "id": 1 } }' )

#add_test( NAME deleteA COMMAND curl -v -XPOST http://localhost:8080/index/delete/A )
#add_test( NAME deteteB COMMAND curl -v -XPOST http://localhost:8080/index/delete/B )
