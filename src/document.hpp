#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "master.hpp"

namespace apps {

class document : public master {

	public:
		document( strusWebService &service );
		
	private:
		void insert_without_id_cmd( );
		void insert_cmd( const std::string id  );
		void update_cmd( const std::string id );
		void delete_cmd( const std::string name, const std::string id );
		void get_cmd( const std::string id );
};

} // namespace apps

#endif

