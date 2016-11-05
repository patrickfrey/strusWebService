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
#include <fstream>
#include <string>
#include <sstream>
#include <streambuf>

#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"

#include <cppcms/json.h>

static int send_request( const std::string &method, const std::string &urlstr, const std::string &data )
{
	std::ostringstream os;
	curlpp::Easy request;
	curlpp::options::WriteStream ws( &os );
	request.setOpt( ws );
	curlpp::options::Url url( urlstr );
	request.setOpt( url );
	//~ request.setOpt( new curlpp::options::Verbose( true ) ); 
	std::cout << urlstr << " " << std::flush;
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
	
	std::istringstream ris( res );
	cppcms::json::value r;
	if( !r.load( ris, true ) ) {
		std::cerr << "ERROR: Illegal JSON received from web service" << std::endl;
		return 1;
	}
	if( r.get<std::string>( "result" ) == "ok" ) {
		std::cerr << "OK" << std::endl;
	} else if( r.get<std::string>( "result" ) == "error" ) {
		std::cerr << "ERROR: " << r.get<std::string>( "err.msg" ) << std::endl;
		return 1;
	} else {
		std::cerr << "ERROR: Unknown value for result '" << r.get<std::string>( "result" ) << "'." << std::endl;
		return 1;
	}
	
	return 0;
}

int main( int argc, char *argv[] )
{
	if( argc != 2 ) {
		std::cerr << "ERROR: Expecting a filename of a request log." << std::endl;
		return 1;
	}
	
	std::ifstream is( argv[1] );
	if( !is.good( ) ) {
		std::cerr << "ERROR: file '" << argv[1] << "' not openable." << std::endl;
		return 1;
	}

	std::string line;
	unsigned int lineNo = 1;
	enum {
		STATE_UNKNOWN,
		STATE_METHOD,
		STATE_URL,
		STATE_DATA
	} state = STATE_UNKNOWN;
	std::string method;
	std::string urlstr;
	std::string data;
	while( std::getline( is, line ) ) {
		switch( state ) {
			case STATE_UNKNOWN:
			case STATE_METHOD:
				if( line == "GET" || line == "POST" ) {
					state = STATE_URL;
					method = line;
				} else {
					std::cerr << "ERROR: expecting GET or POST" << std::endl;
					return 1;
				}
				break;
			
			case STATE_URL:
				urlstr = line;
				state = STATE_DATA;
				break;
			
			case STATE_DATA:
				if( line == "GET" || line == "POST" ) {
					send_request( method, urlstr, data );
					state = STATE_URL;
					data.clear( );
				} else {
					data.append( line );
				}
				break;
		}
		lineNo++;
	}
	if( state == STATE_DATA ) {
		send_request( method, urlstr, data );
	}
	
	return 0;
	
}
