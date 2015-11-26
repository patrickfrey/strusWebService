cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories( "${PROJECT_SOURCE_DIR}/3rdParty/curlpp/include" "${CURL_INCLUDE_DIRS}" )

add_executable( curlJsonTester curlJsonTester.cpp)
target_link_libraries( curlJsonTester "${CURL_LIBRARIES}" curlpp_static)

# other.cpp

add_test( NAME ping COMMAND curlJsonTester ping )
add_test( NAME version COMMAND curlJsonTester version )

# error conditions

add_test( NAME post_not_get COMMAND curlJsonTester post_not_get )
add_test( NAME invalid_json COMMAND curlJsonTester invalid_json )

# index.cpp

add_test( NAME create_index_A_params_missing COMMAND curlJsonTester create_index_A_params_missing )
add_test( NAME create_index_A COMMAND curlJsonTester create_index_A )
add_test( NAME create_index_B COMMAND curlJsonTester create_index_B )

add_test( NAME index_list COMMAND curlJsonTester index_list )

add_test( NAME index_config_A COMMAND curlJsonTester index_config_A )
add_test( NAME index_stats_A COMMAND curlJsonTester index_stats_A )

# document.cpp

add_test( NAME document_url_docid_insert COMMAND curlJsonTester document_url_docid_insert )
add_test( NAME document_payload_docid_insert COMMAND curlJsonTester document_payload_docid_insert )
add_test( NAME document_insert COMMAND curlJsonTester document_insert )

add_test( NAME document_exists_doc1 COMMAND curlJsonTester document_exists_doc1 )
add_test( NAME document_exists_doc2 COMMAND curlJsonTester document_exists_doc2 )
add_test( NAME document_exists_doc3 COMMAND curlJsonTester document_exists_doc3 )

add_test( NAME document_get COMMAND curlJsonTester document_get )

add_test( NAME index_stats_A_after_insert COMMAND curlJsonTester index_stats_A_after_insert )

add_test( NAME document_delete COMMAND curlJsonTester document_delete )

add_test( NAME index_stats_A_after_delete COMMAND curlJsonTester index_stats_A_after_delete )

add_test( NAME document_nonexist_delete COMMAND curlJsonTester document_nonexist_delete )

add_test( NAME index_stats_A_after_nonexist_delete COMMAND curlJsonTester index_stats_A_after_nonexist_delete )

# index.cpp (cleaning up)

add_test( NAME delete_index_A COMMAND curlJsonTester delete_index_A )
add_test( NAME delete_index_B COMMAND curlJsonTester delete_index_B )

