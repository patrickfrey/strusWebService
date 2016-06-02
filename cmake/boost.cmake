set(Boost_USE_MULTITHREADED ON)
find_package( Boost 1.53.0 COMPONENTS atomic QUIET)
if( Boost_ATOMIC_FOUND )
	if(APPLE OR WIN32)
		find_package( Boost 1.53.0 REQUIRED COMPONENTS filesystem thread-mt system date_time atomic-mt program_options timer)
	else()
		find_package( Boost 1.53.0 REQUIRED COMPONENTS filesystem thread system date_time atomic program_options timer)
	endif()
else()
	if(APPLE OR WIN32)
		find_package( Boost 1.53.0 REQUIRED COMPONENTS filesystem thread-mt system date_time program_options timer)
	else()
		find_package( Boost 1.53.0 REQUIRED COMPONENTS filesystem thread system date_time program_options timer)
	endif()
endif()

MESSAGE( STATUS "Boost includes: ${Boost_INCLUDE_DIRS}" )
MESSAGE( STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}" )
MESSAGE( STATUS "Boost libraries: ${Boost_LIBRARIES}" )
