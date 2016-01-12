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
#include "strus/storageTransactionInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/queryProcessorInterface.hpp"

#include <booster/thread.h>

#include "strus/lib/error.hpp"
#include "strus/errorBufferInterface.hpp"

#include "strus/moduleEntryPoint.hpp"

struct StrusIndexContext {
	std::string config;
	strus::DatabaseInterface *dbi;
	strus::StorageInterface *sti;
	strus::DatabaseClientInterface *dbci;
	strus::StorageClientInterface *stci;
	strus::MetaDataReaderInterface *mdri;
	strus::AttributeReaderInterface *atri;
	std::map<std::string, strus::StorageTransactionInterface *> trans_map;
	
	public:
		StrusIndexContext( ) : config( "" ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }

		StrusIndexContext( const std::string &_config )
			: config( _config ),
			dbi( 0 ), sti( 0 ),
			dbci( 0 ), stci( 0 ),
			mdri( 0 ), atri( 0 ) { }			
};

class StrusContext {
	private:
		std::map<std::string, StrusIndexContext *> context_map;	
		booster::mutex mutex;
		FILE *logfile;
		std::vector<const strus::ModuleEntryPoint *> modules;

	public:
		strus::ErrorBufferInterface *g_errorhnd;

	public:
		StrusContext( unsigned int nof_threads, const std::string moduleDir, const std::vector<std::string> modules );
		
		StrusIndexContext *acquire( const std::string &name );
		void release( const std::string &name, StrusIndexContext *ctx );

		std::vector<std::string> getStrusErrorDetails( ) const;
		void registerModules( strus::QueryProcessorInterface *qpi ) const;
};

#endif
