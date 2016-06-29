cmake_minimum_required(VERSION 2.6)

include(cmake/install_prefix.cmake)

project(strusWebService)
set( STRUS_WEB_SERVICE_MAJOR_VERSION 0 )
set( STRUS_WEB_SERVICE_MINOR_VERSION 0 )
set( STRUS_WEB_SERVICE_PATCH_VERSION 3 )
set( STRUS_WEB_SERVICE_VERSION ${STRUS_WEB_SERVICE_MAJOR_VERSION}.${STRUS_WEB_SERVICE_MINOR_VERSION}.${STRUS_WEB_SERVICE_PATCH_VERSION} )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
include( cmake/build_rules.cmake )
include( cmake/standard_targets.cmake )
include( cmake/find_strus_package.cmake )
include( cmake/boost.cmake )
include( cmake/dist.cmake )
include( cmake/FindCURL.cmake )

find_strus_package( core )
find_strus_package( module )

configure_file( "${PROJECT_SOURCE_DIR}/src/version.hpp.in"  "${PROJECT_SOURCE_DIR}/src/version.hpp"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh.in"  "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-Debian_7.0.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-Debian_7.0.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-xUbuntu_12.04.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-xUbuntu_12.04.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL"  @ONLY )

find_library(CPPCMS cppcms)
find_library(BOOSTER booster)

find_path(CPPCMS_INC cppcms/application.h)
find_path(BOOSTER_INC booster/shared_ptr.h)

include_directories(${CPPCMS_INC})
include_directories(${BOOSTER_INC})

add_subdirectory( 3rdParty/curlpp )

enable_testing( )

add_subdirectory( tests )

set(SRC
	src/main.cpp
	src/strusContext.cpp
	src/strusWebService.cpp
	src/master.cpp
	src/other.cpp
	src/index.cpp
	src/document.cpp
	src/query.cpp
	src/transaction.cpp
)

include_directories(
	"${strus_INCLUDE_DIRS}"
)

link_directories(
	"${strus_LIBRARY_DIRS}"
)

add_executable(strusWebService ${SRC})
target_link_libraries(strusWebService ${BOOSTER} ${CPPCMS} strus_storage strus_error strus_database_leveldb strus_queryeval strus_queryproc strus_module strus_error ${Boost_LIBRARIES})

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
