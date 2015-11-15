cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# TODO: make a C command doing the starting in the backgroud,
# the curl sending and comapring the result in JSON
add_test( NAME ping COMMAND curl http://localhost:8080/ping )

add_test( NAME version COMMAND curl http://localhost:8080/version )

add_test( NAME createA COMMAND curl http://localhost:8080/index/create/A )
add_test( NAME createB COMMAND curl http://localhost:8080/index/create/B )

add_test( NAME statsA COMMAND curl http://localhost:8080/index/stats/A )
add_test( NAME statsB COMMAND curl http://localhost:8080/index/stats/B )

add_test( NAME list COMMAND curl http://localhost:8080/index/list )

add_test( NAME deleteA COMMAND curl http://localhost:8080/index/delete/A )
add_test( NAME deteteB COMMAND curl http://localhost:8080/index/delete/B )
