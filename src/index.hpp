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
		index( strusWebService &service );
		virtual ~index( );
		
	protected:
		strus::ErrorBufferInterface *g_errorhnd;
		void prepare_strus_environment( );
		strus::DatabaseInterface *dbi;
		strus::StorageInterface *sti;
		
	private:
		void create_cmd( const std::string name );
		void delete_cmd( const std::string name );
};

} // namespace apps

#endif
