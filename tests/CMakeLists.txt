cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_test( NAME ping COMMAND curl http://localhost:8080/ping )
