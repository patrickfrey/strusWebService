#include "strusContext.hpp"

#include <unistd.h>

#include <boost/algorithm/string.hpp>

StrusContext::StrusContext( unsigned int nof_threads )
	: context_map( )
{
	// implementing a ErrorBufferInterface is not really possible!
	// redirecting the output to a tmpfile and reading and rewinding that
	// one periodically seems to be the only hacky option right now
	logfile = std::tmpfile( );
	
	g_errorhnd = strus::createErrorBuffer_standard( logfile, nof_threads );
}

StrusConnectionContext *StrusContext::acquire( const std::string &name )
{
	std::map<std::string, StrusConnectionContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		return 0;
	} else {
		return it->second;
	}
	mutex.lock( );
}

void StrusContext::release( const std::string &name, StrusConnectionContext *ctx )
{
	std::map<std::string, StrusConnectionContext *>::iterator it;
	it = context_map.find( name );
	if( it == context_map.end( ) ) {
		context_map[name] = ctx;
	} else {
		it->second = ctx;
	}
	mutex.unlock( );
}

std::vector<std::string> StrusContext::getStrusErrorDetails( ) const
{
	char buf[512];
	std::vector<std::string> v;
	
	long pos = ftell( logfile );
	if( pos > 0 ) {
		fseek( logfile, 0 , SEEK_SET );
		while( fgets( buf, sizeof( buf ), logfile ) != 0 ) {
			std::string s( buf );
			boost::trim_right_if( s, boost::is_any_of( "\r\n" ) );
			v.push_back( s );
		}
		(void)ftruncate( fileno( logfile ), 0 );
		fseek( logfile, 0 , SEEK_SET );
	}
	
	return v;
}
