cmake_minimum_required(VERSION 2.6)

include(cmake/install_prefix.cmake)

project(strusWebService)
set( STRUS_WEB_SERVICE_MAJOR_VERSION 0 )
set( STRUS_WEB_SERVICE_MINOR_VERSION 0 )
set( STRUS_WEB_SERVICE_PATCH_VERSION 1 )
set( STRUS_WEB_SERVICE_VERSION ${STRUS_WEB_SERVICE_MAJOR_VERSION}.${STRUS_WEB_SERVICE_MINOR_VERSION}.${STRUS_WEB_SERVICE_PATCH_VERSION} )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
include( cmake/standard_targets.cmake )
include( cmake/find_strus_package.cmake )

find_strus_package( core )

find_library(CPPCMS cppcms)
find_library(BOOSTER booster)

find_path(CPPCMS_INC cppcms/application.h)
find_path(BOOSTER_INC booster/shared_ptr.h)

include_directories(${CPPCMS_INC})
include_directories(${BOOSTER_INC})

set(SRC
	src/main.cpp
	src/strusWebService.cpp
	src/master.cpp
	src/ping.cpp
)

include_directories(
	"${strus_INCLUDE_DIRS}"
)

link_directories(
	"${strus_LIBRARY_DIRS}"
)

add_executable(strusWebService ${SRC})
target_link_libraries(strusWebService ${BOOSTER} ${CPPCMS} strus_storage)

ADD_CUSTOM_TARGET(run @echo runs the web service)
ADD_CUSTOM_COMMAND(
  COMMENT "run"
  COMMAND strusWebService
  ARGS -c ./config.js
  TARGET run
)
