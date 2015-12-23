#ifndef STRUS_CONTEXT_HPP
#define STRUS_CONTEXT_HPP

#include <map>
#include <string>
#include <vector>

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"

#include <booster/thread.h>

#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"

struct StrusConnectionContext {
	std::string config;
	strus::DatabaseInterface *dbi;
	strus::StorageInterface *sti;
	strus::DatabaseClientInterface *dbci;
	strus::StorageClientInterface *stci;
	strus::MetaDataReaderInterface *mdri;
	strus::AttributeReaderInterface *atri;
	
	public:
		StrusConnectionContext( ) : config( "" ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }

		StrusConnectionContext( const std::string &_config )
			: config( _config ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }			
};

class StrusContext {
	private:
		std::map<std::string, StrusConnectionContext *> context_map;	
		booster::mutex mutex;
		FILE *logfile;

	public:
		strus::ErrorBufferInterface *g_errorhnd;

	public:
		StrusContext( unsigned int nof_threads );
		
		StrusConnectionContext *acquire( const std::string &name );
		void release( const std::string &name, StrusConnectionContext *ctx );

		std::vector<std::string> getStrusErrorDetails( ) const;

};

#endif
