include_directories(
	"${LIBBOOSTER_INCLUDE_DIR}"
	"${LIBCPPCMS_INCLUDE_DIR}"
)

link_directories(
)

add_executable( testCppCmsHelloWorld testCppCmsHelloWorld.cpp )
target_link_libraries( testCppCmsHelloWorld ${Boost_LIBRARIES} ${LIBCPPCMS_LIBRARY} ${LIBBOOSTER_LIBRARY} )

