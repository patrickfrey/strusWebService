#ifndef MASTER_HPP
#define MASTER_HPP

#include <cppcms/application.h>
#include <cppcms/json.h>

#include "strus/lib/error.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/errorBufferInterface.hpp"

#include "version.hpp"
#include "error_codes.hpp"

#include <vector>
#include <string>
#include <cstdio>

namespace apps {

class strusWebService;

class master : public cppcms::application {

	private:
		FILE *logfile;
	
	protected:
		strus::ErrorBufferInterface *g_errorhnd;
		strusWebService &service;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	public:
		master( strusWebService &service );
		void register_common_pages( );
		void prepare_strus_environment( );
		void close_strus_environment( );

	protected:
		void report_ok( );
		void report_ok( cppcms::json::value &j );
		void report_error( unsigned int code, const std::string &msg );
		void not_found_404( );
		std::vector<std::string> handle_strus_errors( );
		bool ensure_post( );
		bool ensure_json_request( );
};

} // namespace apps

#endif
