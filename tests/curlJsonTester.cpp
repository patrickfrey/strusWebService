#include <iostream>

#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"

int main( int argc, char *argv[] )
{
	if( argc != 2 ) {
		std::cerr << "ERROR: Expecting a test name." << std::endl;
		return 1;
	}
	
	std::string testname( argv[1] );
	
	std::cout << "Executing test '" << testname << "'" << std::endl;
	
	std::cout << curlpp::options::Url( "http://www.eurospider.com" );

	return 0;
}
