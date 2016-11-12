/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "strusIndexContext.hpp"

#include <booster/log.h>

StrusIndexContext::StrusIndexContext( ) : name( "" ), config( "" ),
	dbi( 0 ), sti( 0 ), stci( 0 ),
	mdri( 0 ), atri( 0 )
{
}

StrusIndexContext::StrusIndexContext( const std::string &_name, const std::string &_config )
	: name( _name ), config( _config ),
	dbi( 0 ), sti( 0 ), stci( 0 ),
	mdri( 0 ), atri( 0 )
{
}
	
StrusIndexContext::~StrusIndexContext( )
{
	abortAllRunningTransactions( );
	
	if( atri != 0 ) delete atri;
	if( mdri != 0 ) delete mdri;
	if( stci != 0 ) delete stci;
	if( sti != 0 ) delete sti;
	if( dbi != 0 ) delete dbi;
}

void StrusIndexContext::read_lock( )
{
	mutex.lock( );
}

void StrusIndexContext::write_lock( )
{
	mutex.lock( );
}

void StrusIndexContext::unlock( )
{
	mutex.unlock( );
}

void StrusIndexContext::abortAllRunningTransactions( )
{
	for( std::map<std::string, StrusTransactionInfo>::const_iterator it = trans_map.begin( ); it != trans_map.end( ); it++ ) {
		BOOSTER_INFO( PACKAGE ) << "forcing rollback on transaction '" << it->first << "' in index '" << name << "'";
		it->second.stti->rollback( );
		delete it->second.stti;
	}
}

void StrusIndexContext::terminateIdleTransactions( unsigned int max_livetime )
{
	for( std::map<std::string, StrusTransactionInfo>::iterator it = trans_map.begin( ); it != trans_map.end( ); it++ ) {	
		unsigned int age = time( 0 ) - it->second.last_used;
		BOOSTER_DEBUG( PACKAGE ) << "transaction '" << it->first << "' has age " << age;
		if( age > max_livetime ) {
			BOOSTER_WARNING( PACKAGE ) << "forcing rollback on idle transaction '" << it->first << "' (age: " << age << " seconds)";
			it->second.stti->rollback( );
			delete it->second.stti;
			trans_map.erase( it );
		}
	}
}
