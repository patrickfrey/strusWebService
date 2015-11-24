#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "master.hpp"

#include <cppcms/json.h>

struct DocumentInsertRequest {
};

namespace apps {

class document : public master {

	public:
		document( strusWebService &service );
		
	private:
		void insert_at_cmd( const std::string name, const std::string id  );
		void insert_cmd( const std::string name );
		void update_cmd( const std::string name, const std::string id );
		void delete_cmd( const std::string name, const std::string id );
		void get_cmd( const std::string name, const std::string id );
		void exists_cmd( const std::string name, const std::string id );
};

} // namespace apps

namespace cppcms {
	namespace json {

template<>
struct traits<DocumentInsertRequest> {
	
	static DocumentInsertRequest get( value const &v )
	{
		DocumentInsertRequest d;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		return d;
	}
	
	static void set( value &v, DocumentInsertRequest const &d )
	{
	}
};

} } // namespace cppcms::json

#endif

