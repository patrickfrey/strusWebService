set( source_files
	webRequestLogger.cpp
	strusWebService.cpp
	configUtils.cpp
	applicationImpl.cpp
	serviceClosure.cpp
	${WEBSERVICE_GEN_SOURCE_DIR}/internationalization.cpp
)

include_directories(
	"${Boost_INCLUDE_DIRS}"
	"${strus_INCLUDE_DIRS}"
	"${WEBSERVICE_SOURCE_DIRS}"
	"${LIBBOOSTER_INCLUDE_DIR}"
	"${LIBCPPCMS_INCLUDE_DIR}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${strusbindings_LIBRARY_DIRS}"
	"${strusmodule_LIBRARY_DIRS}"
	"${strusbase_LIBRARY_DIRS}"
	"${WEBSERVICE_LIBRARY_DIRS}"
)

add_cppcheck( strusWebService ${source_files} )

add_executable( strusWebService ${source_files})
target_link_libraries( strusWebService ${Boost_LIBRARIES} ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} strus_webrequest strus_bindings strus_bindings_description strus_error strus_base strus_module ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

# FreeBSD needs kernel data access library for libuv (-libkvm)
find_library( LIBKVM_LIBRARIES kvm )
if(LIBKVM_LIBRARIES)
	target_link_libraries( strusWebService ${LIBKVM_LIBRARIES} )
endif()

install( TARGETS strusWebService RUNTIME DESTINATION sbin )

ADD_CUSTOM_TARGET(run @echo runs the web service)
ADD_CUSTOM_COMMAND(
	COMMENT "run"
	COMMAND strusWebService
	ARGS -v -c ${PROJECT_SOURCE_DIR}/config.js
	TARGET run
)

ADD_CUSTOM_TARGET(debugrun @echo runs the web service in debugger)
ADD_CUSTOM_COMMAND(
	COMMENT "debugrun"
	COMMAND gdb
	ARGS --args strusWebService -v -c ${PROJECT_SOURCE_DIR}/config.js
	TARGET debugrun
)

