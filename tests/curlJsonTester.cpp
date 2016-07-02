/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <streambuf>

#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"

#include <cppcms/json.h>

int main( int argc, char *argv[] )
{
	if( argc != 2 ) {
		std::cerr << "ERROR: Expecting a test name." << std::endl;
		return 1;
	}
	
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

	std::cout << "METHOD: " << method << std::endl;
	std::cout << "URL: " << urlstr << std::endl;
	std::cout << "DATA: " << data << std::endl;

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
	
	std::ostringstream ss3;
	ss3 << testname << ".res";
	std::ofstream resfile( ss3.str( ).c_str( ) );
	if( !resfile.good( ) ) {
		std::cerr << "ERROR: Result file '" << ss3.str( ) << "' not openable." << std::endl;
		return 1;
	}
	resfile << res;
	
	std::cout << "MUST: " << must << std::endl;
	std::cout << "RES : " << res << std::endl;

	std::istringstream mis( must );
	cppcms::json::value pmust;
	if( !pmust.load( mis, true ) ) {
		std::cerr << "ERROR: Illegal JSON in must file" << std::endl;
		return 1;
	}
	
	std::istringstream ris( res );
	cppcms::json::value pres;
	if( !pres.load( ris, true ) ) {
		std::cerr << "ERROR: Illegal JSON in res file" << std::endl;
		return 1;
	}
	
	// filter out execution_time in result, in the future we may measure
	// that the execution time is withing known limits..
	if( pres.type( "execution_time" ) == cppcms::json::is_number ) {
		pres.set( "execution_time", 0 );
	}
	
	std::cout << "MUST (JSON): " << pmust << std::endl;
	std::cout << "RES (JSON) : " << pres << std::endl;
	
	std::ostringstream omust;
	pmust.save( omust );
	
	std::ostringstream ores;
	pres.save( ores );
					
	if( ores.str( ).compare( omust.str( ) ) != 0 ) {
		return 1;
	}
	
	return 0;
}
