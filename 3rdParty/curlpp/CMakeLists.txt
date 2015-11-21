cmake_minimum_required(VERSION 2.6)

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

add_library( curlpp_static STATIC ${curlpp_source_files} )

