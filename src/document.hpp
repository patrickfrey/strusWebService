#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "master.hpp"

#include <cppcms/json.h>

#include <vector>
#include <utility>

struct DocumentInsertRequest {
	std::string docid;
	std::vector<std::pair<std::string, std::string> > attributes;
};

namespace apps {

class document : public master {

	public:
		document( strusWebService &service );
		
	private:
		void insert_url_cmd( const std::string name, const std::string id );
		void insert_payload_cmd( const std::string name );
		void insert_cmd( const std::string name, const std::string id, bool docid_id_url );
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
		// TODO: should we also accept int, float? how can we fall back?
		d.docid = v.get<std::string>( "docid", "" );
		d.attributes = v.get<std::vector<std::pair<std::string, std::string> > >( "attributes" );
		return d;
	}
	
	static void set( value &v, DocumentInsertRequest const &d )
	{
		v.set( "docid", d.docid );
	}
};

template<>
struct traits<std::pair<std::string, std::string> > {
	
	static std::pair<std::string, std::string> get( value const &v )
	{
		std::pair<std::string, std::string> p;
		
		p.first = v.get<std::string>( "key" );
		p.second = v.get<std::string>( "value" );
		
		return p;
	}
	
	static void set( value &v, std::pair<std::string, std::string> &p )
	{
		v.set( "key", p.first );
		v.set( "value", p.second );
	}
};

} } // namespace cppcms::json

#endif

