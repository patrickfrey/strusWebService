cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/3rdParty/curlpp/include" "${CURL_INCLUDE_DIRS}"
	"${strus_INCLUDE_DIRS}"
)

link_directories(
	"${strus_LIBRARY_DIRS}"
)

add_executable( queryStructTester queryStructTester.cpp)
target_link_libraries( queryStructTester ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY})

add_executable( curlJsonTester curlJsonTester.cpp)
target_link_libraries( curlJsonTester "${CURL_LIBRARIES}" ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} curlpp_static)

add_executable( curlFeeder curlFeeder.cpp)
target_link_libraries( curlFeeder "${CURL_LIBRARIES}" ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} curlpp_static)

# low-level tests

add_test( NAME query_struct COMMAND queryStructTester )

# other.cpp

add_test( NAME ping COMMAND curlJsonTester ping )
add_test( NAME version COMMAND curlJsonTester version )

# error conditions

add_test( NAME post_not_get COMMAND curlJsonTester post_not_get )
add_test( NAME invalid_json COMMAND curlJsonTester invalid_json )

# index.cpp

add_test( NAME create_index_params_missing COMMAND curlJsonTester create_index_params_missing )
add_test( NAME create_index_A COMMAND curlJsonTester create_index_A )
add_test( NAME create_index_B COMMAND curlJsonTester create_index_B )

add_test( NAME index_list COMMAND curlJsonTester index_list )

add_test( NAME index_stats_A COMMAND curlJsonTester index_stats_A )

# index creation with parameter (create, verify metatable, delete index C)
add_test( NAME create_index_C COMMAND curlJsonTester create_index_C )
add_test( NAME index_config_C COMMAND curlJsonTester index_config_C )
add_test( NAME delete_index_C COMMAND curlJsonTester delete_index_C )

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

add_test( NAME index_config_A COMMAND curlJsonTester index_config_A )

# transaction.cpp

add_test( NAME transaction_begin COMMAND curlJsonTester transaction_begin )

add_test( NAME transaction_insert_doc1 COMMAND curlJsonTester transaction_insert_doc1 )

add_test( NAME transaction_insert_doc2 COMMAND curlJsonTester transaction_insert_doc2 )

add_test( NAME transaction_insert_doc3 COMMAND curlJsonTester transaction_insert_doc3 )

add_test( NAME transaction_list_before_commit COMMAND curlJsonTester transaction_list_before_commit )

add_test( NAME transaction_commit COMMAND curlJsonTester transaction_commit )

add_test( NAME transaction_list_after_commit COMMAND curlJsonTester transaction_list_after_commit )

add_test( NAME transaction_delete_begin COMMAND curlJsonTester transaction_begin )

add_test( NAME transaction_delete_doc2 COMMAND curlJsonTester transaction_delete_doc2 )

add_test( NAME transaction_delete_commit COMMAND curlJsonTester transaction_commit )

# query.cpp

add_test( NAME query_in_url COMMAND curlJsonTester query_in_url )
add_test( NAME query_A COMMAND curlJsonTester query_A )
add_test( NAME query_subexpression_A COMMAND curlJsonTester query_subexpression_A )
add_test( NAME query_summarizer COMMAND curlJsonTester query_summarizer )

# index.cpp (cleaning up)

add_test( NAME delete_index_A COMMAND curlJsonTester delete_index_A )
add_test( NAME delete_index_B COMMAND curlJsonTester delete_index_B )

