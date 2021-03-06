cmake_minimum_required(VERSION 2.8)

include(cmake/install_prefix.cmake)

project(strusWebService)
set( STRUS_MAJOR_VERSION 0 )
set( STRUS_MINOR_VERSION 17 )
set( STRUS_PATCH_VERSION 0 )
set( STRUS_VERSION ${STRUS_MAJOR_VERSION}.${STRUS_MINOR_VERSION}.${STRUS_PATCH_VERSION} )

if(NOT STRUS_ALL)
set( CMAKE_MODULE_PATH  ${PROJECT_SOURCE_DIR}/cmake )
include( cmake/cxx11.cmake )
check_for_cxx11_compiler( HAVE_CXX11 )
include( cmake/dist.cmake )
include( cmake/LinuxDistribution.cmake )
include( cmake/build_rules.cmake )
include( cmake/boost.cmake)
include( cmake/FindCURL.cmake )
include( cmake/FindCppCms.cmake )
include( cmake/find_strus_package.cmake )
include( cmake/link_rules.cmake )
include( cmake/standard_targets.cmake )
include( cmake/intl.cmake )
include( cmake/libdl.cmake )
include( cmake/report_build_settings.cmake )
include( cmake/cppcheck.cmake )
include( cmake/find_strus_package.cmake )
include( cmake/destination.cmake )
endif(NOT STRUS_ALL)

find_strus_package( base )
find_strus_package( core )
find_strus_package( analyzer )
find_strus_package( module )
find_strus_package( trace )
find_strus_package( rpc )
find_strus_package( bindings )

if( APPLE OR WIN32 )
	set( PROJECT_BIN_AFFIX "${CMAKE_BUILD_TYPE}/" )
else( APPLE OR WIN32 )
	set( PROJECT_BIN_AFFIX "" )
endif( APPLE OR WIN32 )

# Module path:
if (NOT STRUS_MODULE_PATH)
set( STRUS_MODULE_PATH  "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus/modules" )
endif (NOT STRUS_MODULE_PATH)
MESSAGE( STATUS  "Strus module path: ${STRUS_MODULE_PATH}" )

set( STRUS_GETTEXT_PACKAGE "strus-dom" )	#... 1st parameter of bindtextdomain(...)
set( STRUS_GETTEXT_LOCALEDIR "" )			#... 2nd parameter of bindtextdomain(...)

# Path declarations
set( WEBSERVICE_SOURCE_DIRS                 "${CMAKE_CURRENT_BINARY_DIR}/src"  "${PROJECT_SOURCE_DIR}/src" )
set( WEBSERVICE_LIBRARY_DIRS                 "${CMAKE_CURRENT_BINARY_DIR}/src" )
set( WEBSERVICE_GEN_SOURCE_DIR          "${CMAKE_CURRENT_BINARY_DIR}/src" )
set( WEBSERVICE_CONFIG_DIR                    "${CMAKE_CURRENT_BINARY_DIR}" )


# Configure files
set( STRUS_SERVER_PORT 8080 )
set( STRUS_SERVER_ID  "strus" )

configure_file( "${PROJECT_SOURCE_DIR}/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config.js"  @ONLY )
install( FILES "${CMAKE_CURRENT_BINARY_DIR}/config.js" DESTINATION "${STRUS_DESTINATION_CONFIG}" COMPONENT config )

configure_file( "${PROJECT_SOURCE_DIR}/src/versionWebService.hpp.in"  "${CMAKE_CURRENT_BINARY_DIR}/src/versionWebService.hpp"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/src/internationalization.cpp.in"  "${CMAKE_CURRENT_BINARY_DIR}/src/internationalization.cpp"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh.in"  "${PROJECT_SOURCE_DIR}/dist/obs/deploy_to_obs.sh"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/PKGBUILD"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/archlinux/strusWebService.conf.in"  "${PROJECT_SOURCE_DIR}/dist/archlinux/strusWebService.conf"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-xUbuntu_14.04.dsc.in"  "${PROJECT_SOURCE_DIR}/dist/obs/struswebservice-xUbuntu_14.04.dsc"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/struswebservice.spec"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-RHEL"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SUSE.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SUSE"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SLES11.in"  "${PROJECT_SOURCE_DIR}/dist/redhat/strusWebService.conf-SLES11"  @ONLY )
configure_file( "${PROJECT_SOURCE_DIR}/dist/debian/strusWebService.conf.in"  "${PROJECT_SOURCE_DIR}/dist/debian/strusWebService.conf"  @ONLY )

add_subdirectory( 3rdParty/curlpp )
enable_testing( )
add_subdirectory( src )
add_subdirectory( client_curl )
add_subdirectory( tests )

