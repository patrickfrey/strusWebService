cmake_minimum_required(VERSION 2.6)

include(cmake/install_prefix.cmake)

project(strusWebService)
set( STRUS_WEB_SERVICE_MAJOR_VERSION 0 )
set( STRUS_WEB_SERVICE_MINOR_VERSION 0 )
set( STRUS_WEB_SERVICE_PATCH_VERSION 4 )
set( STRUS_WEB_SERVICE_VERSION ${STRUS_WEB_SERVICE_MAJOR_VERSION}.${STRUS_WEB_SERVICE_MINOR_VERSION}.${STRUS_WEB_SERVICE_PATCH_VERSION} )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
include( cmake/build_rules.cmake )
include( cmake/standard_targets.cmake )
include( cmake/find_strus_package.cmake )
include( cmake/boost.cmake )
include( cmake/dist.cmake )
include( cmake/FindCURL.cmake )
include( cmake/FindCppCms.cmake )

find_strus_package( core )
find_strus_package( module )

configure_file( "${PROJECT_SOURCE_DIR}/src/version.hpp.in"  "${PROJECT_SOURCE_DIR}/src/version.hpp"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh.in"  "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SUSE.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SUSE"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SLES11.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SLES11"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/strusWebService.conf.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/strusWebService.conf"  @ONLY )

add_subdirectory( 3rdParty/curlpp )

enable_testing( )

add_subdirectory( tests )

set(SRC
	src/main.cpp
	src/strusIndexContext.cpp
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
	"${Boost_INCLUDE_DIRS}"
	"${strus_INCLUDE_DIRS}"
	"${LIBBOOSTER_INCLUDE_DIR}"
	"${LIBCPPCMS_INCLUDE_DIR}"
)

link_directories(
	"${Boost_LIBRARY_DIRS}"
	"${strus_LIBRARY_DIRS}"
)

add_executable(strusWebService ${SRC})
target_link_libraries(strusWebService ${Boost_LIBRARIES} ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} strus_storage strus_error strus_database_leveldb strus_queryeval strus_queryproc strus_module strus_error ${Boost_LIBRARIES})

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
