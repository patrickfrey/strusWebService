#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <streambuf>

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
	std::ostringstream ss;
	std::string testname( argv[1] );
	ss << testname << ".url";
	std::ifstream testurlfile( ss.str( ).c_str( ) );
	if( !testurlfile.good( ) ) {
		std::cerr << "ERROR: URL file '" << ss.str( ) << "' not openable." << std::endl;
		return 1;
	}
	std::string line;
	std::getline( testurlfile, line );
	testurlfile.close( );

	std::ostringstream os;
	curlpp::Easy request;
	curlpp::options::WriteStream ws( &os );
	request.setOpt( ws );
	curlpp::options::Url url( line );
	request.setOpt( url );
	request.perform( );
	std::string res = os.str( );
	
	std::ostringstream ss2;
	ss2 << testname << ".must";
	std::ifstream mustfile( ss2.str( ).c_str( ) );
	if( !mustfile.good( ) ) {
		std::cerr << "ERROR: Must file '" << ss2.str( ) << "' not openable." << std::endl;
		return 1;
	}
	std::string must;
	mustfile.seekg( 0, std::ios::end );
	must.reserve( mustfile.tellg( ) );
	mustfile.seekg( 0, std::ios::beg );
	must.assign( std::istreambuf_iterator<char>( mustfile ), std::istreambuf_iterator<char>( ) );            	
	mustfile.close( );
	
	std::cout << "MUST: " << must << std::endl;
	std::cout << "RES : " << res << std::endl;
	
	if( res.compare( must ) != 0 ) {
		return 1;
	}
	
	return 0;
}
