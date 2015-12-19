#include "strusContext.hpp"

StrusContext::StrusContext( )
	: context_map( )
{
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
}
