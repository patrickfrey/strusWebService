set( source_files
	strusWebServiceClient.cpp
	${WEBSERVICE_GEN_SOURCE_DIR}/internationalization.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
	"${strusrpc_INCLUDE_DIRS}"
	"${strusbindings_INCLUDE_DIRS}"
	"${WEBSERVICE_SOURCE_DIRS}"
	"${CURL_INCLUDE_DIRS}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
	"${strusrpc_LIBRARY_DIRS}"
	"${strusbindings_LIBRARY_DIRS}"
	"${strusmodule_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
)

add_cppcheck( strusWebServiceClient ${source_files} )

add_executable( strusWebServiceClient ${source_files})
target_link_libraries( strusWebServiceClient ${Boost_LIBRARIES} ${CURL_LIBRARIES} strus_error strus_base strus_webrequest ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

# FreeBSD needs kernel data access library for libuv (-libkvm)
find_library( LIBKVM_LIBRARIES kvm )
if(LIBKVM_LIBRARIES)
	target_link_libraries( strusWebServiceClient ${LIBKVM_LIBRARIES} )
endif()

install( TARGETS strusWebServiceClient RUNTIME DESTINATION sbin )

