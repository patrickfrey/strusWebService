#include <iostream>
#include <sstream>
#include <fstream>

#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"

int main( int argc, char *argv[] )
{
	if( argc != 2 ) {
		std::cerr << "ERROR: Expecting a test name." << std::endl;
		return 1;
	}
	
	// read URL to test
	std::ostringstream os;
	std::string testname( argv[1] );
	os << testname << ".url";
	std::ifstream testurlfile( os.str( ).c_str( ) );
	if( !testurlfile.good( ) ) {
		std::cerr << "ERROR: URL file '" << os.str( ) << "' not openable." << std::endl;
		return 1;
	}
	std::string line;
	std::getline( testurlfile, line );
	testurlfile.close( );
		
	std::cout << "URL: " << line << std::endl;
	os.clear( );
	curlpp::Easy request;
	curlpp::options::WriteStream ws( &os );
	request.setOpt( ws );
	curlpp::options::Url url( line );
	request.setOpt( url );
	request.perform( );
	
	std::string result = os.str( );
	
	std::cout << result << std::endl;
	
	return 0;
}
