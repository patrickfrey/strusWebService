#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "other.hpp"
#include "index.hpp"
#include "document.hpp"
#include "query.hpp"

#include <cppcms/application.h>  

#include <map>

#include "strus/lib/error.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/errorBufferInterface.hpp"

struct strusContext {
	strus::DatabaseInterface *dbi;
	strus::StorageInterface *sti;
};

namespace apps {

class strusWebService : public cppcms::application {

	private:
		FILE *logfile;
		strus::ErrorBufferInterface *g_errorhnd;
		std::map<std::string, struct strusContext> context_map;

	public:
		apps::master master;
		apps::other other;
		apps::index index;
		apps::document document;
		apps::query query;

	public:
		strusWebService( cppcms::service &srv );
		strus::DatabaseInterface *getDataBaseInterface( const std::string &name );
		strus::StorageInterface *getStorageInterface( const std::string &name );
		std::string getLastStrusError( );
		std::vector<std::string> getStrusErrorDetails( );
		std::string getStorageDirectory( const std::string &base_storage_dir, const std::string &name );
		std::string getStorageConfig( const std::string &base_storage_dir, const struct StorageCreateParameters params, const std::string &name );
};

} // namespace apps

#endif
