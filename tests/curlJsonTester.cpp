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
	std::string method;
	std::string urlstr;
	std::getline( testurlfile, method );
	std::getline( testurlfile, urlstr );
	std::string line;
	std::string data;
	while( std::getline( testurlfile, line ) ) {
		data.append( line );
	}
	testurlfile.close( );

	std::ostringstream os;
	curlpp::Easy request;
	curlpp::options::WriteStream ws( &os );
	request.setOpt( ws );
	curlpp::options::Url url( urlstr );
	request.setOpt( url );
    request.setOpt( new curlpp::options::Verbose( true ) ); 
    std::list<std::string> header;
	std::istringstream is( data );
    if( method.compare( "POST" ) == 0 ) {
		request.setOpt( new curlpp::options::Post( true ) );
		std::ostringstream ss3;
		ss3 << "Content-Length: " << data.size( );
		header.push_back( "Content-Type: application/json" );
		header.push_back( ss3.str( ) );
		request.setOpt( new curlpp::options::ReadStream( &is ) );
		request.setOpt( new curlpp::options::InfileSize( data.size( ) ) );
		request.setOpt( new curlpp::options::HttpHeader( header ) ); 


  //~ std::istringstream myStream(argv[2]);
  //~ int size = myStream.str().size();

      //~ request.setOpt(new ReadStream(&myStream));
      //~ request.setOpt(new InfileSize(size));
      //~ request.setOpt(new Upload(true));

	}
    
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
