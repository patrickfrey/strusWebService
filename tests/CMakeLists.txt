cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# TODO: doens't get executed! No clue why!! 
add_test( NAME ping COMMAND curl http://localhost:8080/ping )
add_test( ping2 curl http://localhost:8080/ping )

# curl http://localhost:8080/ping
# curl http://localhost:8080/index/create/coll5
# curl http://localhost:8080/index/delete/coll5
