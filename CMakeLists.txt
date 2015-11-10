cmake_minimum_required(VERSION 2.6)

project(strusWebService)
set( STRUS_WEB_SERVICE_MAJOR_VERSION 0 )
set( STRUS_WEB_SERVICE_MINOR_VERSION 0 )
set( STRUS_WEB_SERVICE_PATCH_VERSION 1 )
set( STRUS_WEB_SERVICE_VERSION ${STRUS_WEB_SERVICE_MAJOR_VERSION}.${STRUS_WEB_SERVICE_MINOR_VERSION}.${STRUS_WEB_SERVICE_PATCH_VERSION} )

include( cmake/find_strus_package.cmake )

find_library(CPPCMS cppcms)
find_library(BOOSTER booster)

find_path(CPPCMS_INC cppcms/application.h)
find_path(BOOSTER_INC booster/shared_ptr.h)

include_directories(${CPPCMS_INC})
include_directories(${BOOSTER_INC})
include( cmake/find_strus_package.cmake )
