#ifndef MASTER_HPP
#define MASTER_HPP

#include <cppcms/application.h>
#include <cppcms/json.h>

#include "version.hpp"
#include "error_codes.hpp"

#include <vector>
#include <string>
#include <cstdio>
#include <map>

#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"

namespace apps {

class strusWebService;

class master : public cppcms::application {
	
	protected:
		strusWebService &service;
		bool protocol_pretty_printing;
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	public:
		master( strusWebService &service );
		void register_common_pages( );
		void get_strus_environment( const std::string &name );
		void close_strus_environment( const std::string &name );

	protected:
		void report_ok( );
		void report_ok( cppcms::json::value &j );
		void report_error( unsigned int code, const std::string &msg );
		void not_found_404( );
		bool ensure_post( );
		bool ensure_json_request( );

};

} // namespace apps

#endif
