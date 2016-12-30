cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${PROJECT_SOURCE_DIR}/src"
	"${PROJECT_SOURCE_DIR}/3rdParty/curlpp/include" "${CURL_INCLUDE_DIRS}"
	"${LIBBOOSTER_INCLUDE_DIR}"
	"${LIBCPPCMS_INCLUDE_DIR}"
	"${strus_INCLUDE_DIRS}"
)

link_directories(
	"${strus_LIBRARY_DIRS}"
)

add_executable( queryStructTester queryStructTester.cpp)
target_link_libraries( queryStructTester ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} ${Boost_LIBRARIES})

add_executable( curlJsonTester curlJsonTester.cpp)
target_link_libraries( curlJsonTester "${CURL_LIBRARIES}" ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} curlpp_static)

add_executable( curlFeeder curlFeeder.cpp)
target_link_libraries( curlFeeder "${CURL_LIBRARIES}" ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} curlpp_static)

# low-level tests

add_test( NAME query_struct COMMAND queryStructTester )

# other.cpp

add_test( NAME ping COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" ping )
add_test( NAME version COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" version )

# error conditions

add_test( NAME post_not_get COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" post_not_get )
add_test( NAME invalid_json COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" invalid_json )

# index.cpp

add_test( NAME create_index_params_missing COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" create_index_params_missing )
add_test( NAME create_index_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" create_index_A )
add_test( NAME create_index_B COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" create_index_B )

add_test( NAME index_list COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_list )

add_test( NAME index_stats_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_stats_A )

# index creation with parameter (create, verify metatable, delete index C)
add_test( NAME create_index_C COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" create_index_C )
add_test( NAME index_config_C COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_config_C )
add_test( NAME delete_index_C COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" delete_index_C )

# document.cpp

add_test( NAME document_url_docid_insert COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_url_docid_insert )
add_test( NAME document_payload_docid_insert COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_payload_docid_insert )
add_test( NAME document_insert COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_insert )

add_test( NAME document_exists_doc1 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_exists_doc1 )
add_test( NAME document_exists_doc2 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_exists_doc2 )
add_test( NAME document_exists_doc3 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_exists_doc3 )
 
add_test( NAME document_get COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_get )

add_test( NAME index_stats_A_after_insert COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_stats_A_after_insert )

add_test( NAME document_delete COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_delete )

add_test( NAME index_stats_A_after_delete COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_stats_A_after_delete )

add_test( NAME document_nonexist_delete COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" document_nonexist_delete )

add_test( NAME index_stats_A_after_nonexist_delete COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_stats_A_after_nonexist_delete )

add_test( NAME index_config_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" index_config_A )

# transaction.cpp

add_test( NAME transaction_begin COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_begin )

add_test( NAME transaction_insert_doc1 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_insert_doc1 )

add_test( NAME transaction_insert_doc2 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_insert_doc2 )

add_test( NAME transaction_insert_doc3 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_insert_doc3 )

add_test( NAME transaction_list_before_commit COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_list_before_commit )

add_test( NAME transaction_commit COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_commit )

add_test( NAME transaction_list_after_commit COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_list_after_commit )

add_test( NAME transaction_delete_begin COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_begin )

add_test( NAME transaction_delete_doc2 COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_delete_doc2 )

add_test( NAME transaction_delete_commit COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" transaction_delete_commit )

# query.cpp

add_test( NAME query_in_url COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" query_in_url )
add_test( NAME query_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" query_A )
add_test( NAME query_subexpression_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" query_subexpression_A )
add_test( NAME query_summarizer COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" query_summarizer )

# rename and swapping

add_test( NAME rename_A_D COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" rename_A_D )
add_test( NAME list_after_rename COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" list_after_rename )
add_test( NAME swap_B_D COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" swap_B_D )
add_test( NAME list_after_swap COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" list_after_swap )
add_test( NAME rename_B_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" rename_B_A )
add_test( NAME rename_D_B COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" rename_D_B )
add_test( NAME list_after_end_of_swap_renames COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" list_after_end_of_swap_renames )

# explicit open and close

add_test( NAME explicit_open_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" explicit_open_A )
add_test( NAME explicit_close_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" explicit_close_A )

# index.cpp (cleaning up)

add_test( NAME delete_index_A COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" delete_index_A )
add_test( NAME delete_index_B COMMAND curlJsonTester "${PROJECT_SOURCE_DIR}/tests" delete_index_B )

