#ifndef INDEX_HPP
#define INDEX_HPP

#include "master.hpp"

#include "strus/lib/error.hpp"
#include "strus/databaseInterface.hpp"
#include "strus/storageInterface.hpp"

#include <cppcms/json.h>

namespace apps {

class index : public master {

	public:
		index( strusWebService &service, std::string storage_base_directory );
		virtual ~index( );
		
	protected:
		std::string storage_base_directory;
		strus::ErrorBufferInterface *g_errorhnd;
		
		void prepare_strus_environment( );
		std::string get_storage_directory( const std::string &base_storage_dir, const std::string &name );
		std::string get_storage_config( const std::string &base_storage_dir, const std::string &name );
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	private:
		void create_cmd( const std::string name );
		void delete_cmd( const std::string name );
		void list_cmd( );
};

} // namespace apps

#endif
