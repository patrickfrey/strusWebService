#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "master.hpp"

#include "strus/arithmeticVariant.hpp"

#include <boost/lexical_cast.hpp>

#include <cppcms/json.h>

#include <vector>
#include <utility>

struct DocumentInsertRequest {
	std::string docid;
	std::vector<std::pair<std::string, std::string> > attributes;
	std::vector<std::pair<std::string, strus::ArithmeticVariant> > metadata;
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
		d.metadata = v.get<std::vector<std::pair<std::string, strus::ArithmeticVariant> > >( "metadata" );
		return d;
	}
	
	static void set( value &v, DocumentInsertRequest const &d )
	{
		v.set( "docid", d.docid );
		v.set( "attributes", d.attributes );
		v.set( "metadata", d.metadata );
	}
};

template<>
struct traits<std::pair<std::string, std::string> > {
	
	static std::pair<std::string, std::string> get( value const &v )
	{
		std::pair<std::string, std::string> p;

		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		p.first = v.get<std::string>( "key" );
		p.second = v.get<std::string>( "value" );
		
		return p;
	}
	
	static void set( value &v, std::pair<std::string, std::string> const &p )
	{
		v.set( "key", p.first );
		v.set( "value", p.second );
	}
};

template<>
struct traits<std::pair<std::string, strus::ArithmeticVariant> > {

	template<typename T> static bool is_of_type( value const &v, T )
	{
		try {
			if( v.type( ) == is_number ) {
				T t = boost::lexical_cast<T>( v.number( ) );
				(void)t;
				return true;
			} else if( v.type( ) == is_string ) {
				T t = boost::lexical_cast<T>( v.str( ) );
				(void)t;
				return true;
			} else {
				return false;
			}
		} catch( boost::bad_lexical_cast e ) {
			return false;
		}		
	}
	
	static std::pair<std::string, strus::ArithmeticVariant> get( value const &v )
	{
		std::pair<std::string, strus::ArithmeticVariant> p;
		
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		p.first = v.get<std::string>( "key" );
		value val = v["value"];
		switch( val.type( ) ) {
			case is_boolean:
				// TODO: really? Do we allow this?
				p.second = ( v.get<bool>( "value" ) ) ? 1 : 0;
				break;
			
			case is_string:
				// TODO: really? Do we allow this?
			case is_number:
				if( is_of_type( val, p.second.variant.Int ) ) {
					p.second = v.get<typeof( p.second.variant.Int )>( "value" );
				} else if( is_of_type( val, p.second.variant.UInt ) ) {
					p.second = v.get<typeof( p.second.variant.UInt )>( "value" );
				} else if( is_of_type( val, p.second.variant.Float ) ) {
					p.second = v.get<typeof( p.second.variant.Float )>( "value" );
				} else {
					throw bad_value_cast( );
				}
				break;
				
			case is_undefined:
			case is_null:
			case is_object:
			case is_array:	
			default:
				throw bad_value_cast( );
		}
				
		return p;
	}

	static void set( value &v, std::pair<std::string, strus::ArithmeticVariant> const &p )
	{
		// TODO: implement first, used for document/get introspection
		throw bad_value_cast( );
	}
};

} } // namespace cppcms::json

#endif

