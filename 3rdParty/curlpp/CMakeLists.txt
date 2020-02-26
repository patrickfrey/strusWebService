cmake_minimum_required(VERSION 2.8)

include_directories( "${PROJECT_SOURCE_DIR}/3rdParty/curlpp/include" "${CURL_INCLUDE_DIRS}" )

set( curlpp_source_files
	src/curlpp/Easy.cpp
	src/curlpp/Exception.cpp
	src/curlpp/Form.cpp
	src/curlpp/Info.cpp
	src/curlpp/Infos.cpp
	src/curlpp/Multi.cpp
	src/curlpp/Option.cpp
	src/curlpp/Options.cpp
	src/curlpp/OptionBase.cpp
	src/curlpp/cURLpp.cpp
	src/curlpp/internal/CurlHandle.cpp
	src/curlpp/internal/OptionList.cpp
	src/curlpp/internal/OptionSetter.cpp
	src/curlpp/internal/SList.cpp
	src/utilspp/LifetimeLibrary.cpp
	src/utilspp/PrivateMembers.cpp
)

# Build libcurl with older C++ because of use of deprecated constructs:
string( REPLACE "-std=c++11"  ""  CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} )
string( REPLACE "-std=c++14"  ""  CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} )
string( REPLACE "-std=c++17"  ""  CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} )
string( REPLACE "-std=c++20"  ""  CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS} )
set( CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}" )

add_library( curlpp_static STATIC ${curlpp_source_files} )


