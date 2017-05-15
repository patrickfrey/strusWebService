/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef DOCUMENT_HPP
#define DOCUMENT_HPP

#include "master.hpp"

#include "utils.hpp"

#include "strus/numericVariant.hpp"
#include "strus/index.hpp"

#include <boost/typeof/typeof.hpp>

#include <cppcms/json.h>

#include <vector>
#include <utility>
#include <boost/tuple/tuple.hpp>

struct DocumentRequestBase {
	std::string docid;
};

struct DocumentInsertRequest : public DocumentRequestBase {
	std::vector<std::pair<std::string, std::string> > attributes;
	std::vector<std::pair<std::string, strus::NumericVariant> > metadata;
	std::vector<boost::tuple<std::string, std::string, strus::Index> > forward;
	std::vector<boost::tuple<std::string, std::string, strus::Index> > search;
};

struct DocumentDeleteRequest : public DocumentRequestBase {
};

struct DocumentGetRequest : public DocumentRequestBase {
};

struct DocumentGetAnswer {
	strus::Index docno;
	std::vector<std::pair<std::string, std::string> > attributes;
	std::vector<std::pair<std::string, strus::NumericVariant> > metadata;
	std::vector<boost::tuple<std::string, std::string, strus::Index> > forward;
	std::vector<boost::tuple<std::string, std::string, strus::Index> > search;
};
	
namespace apps {

class document : public master {

	public:
		document( strusWebService &service );
		
	private:
		void insert_url_cmd( const std::string name, const std::string id );
		void insert_payload_cmd( const std::string name );
		void insert_cmd( const std::string name, const std::string id, bool docid_in_url );
		void update_url_cmd( const std::string name, const std::string id );
		void update_payload_cmd( const std::string name );
		void update_cmd( const std::string name, const std::string id, bool docid_in_url );
		void delete_url_cmd( const std::string name, const std::string id );
		void delete_payload_cmd( const std::string name );
		void delete_cmd( const std::string name, const std::string id, bool docid_in_url );
		void get_url_cmd( const std::string name, const std::string id );
		void get_payload_cmd( const std::string name );
		void get_cmd( const std::string name, const std::string id, bool docid_in_url );
		void exists_url_cmd( const std::string name, const std::string id );
		void exists_payload_cmd( const std::string name );
		void exists_cmd( const std::string name, const std::string id, bool docid_in_url );
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
		d.metadata = v.get<std::vector<std::pair<std::string, strus::NumericVariant> > >( "metadata" );
		d.forward = v.get<std::vector<boost::tuple<std::string, std::string, strus::Index> > >( "forward" );
		d.search = v.get<std::vector<boost::tuple<std::string, std::string, strus::Index> > >( "search" );
		return d;
	}
	
	static void set( value &v, DocumentInsertRequest const &d )
	{
		v.set( "docid", d.docid );
		v.set( "attributes", d.attributes );
		v.set( "metadata", d.metadata );
		v.set( "forward", d.forward );
		v.set( "search", d.search );
	}
};

template<>
struct traits<DocumentRequestBase> {

	static DocumentRequestBase get( value const &v )
	{
		DocumentRequestBase d;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		d.docid = v.get<std::string>( "docid", "" );	

		return d;	
	}

	static void set( value &v, DocumentRequestBase const &d )
	{
		v.set( "docid", d.docid );
	}
};

template<>
struct traits<DocumentDeleteRequest> {

	static DocumentDeleteRequest get( value const &v )
	{
		DocumentDeleteRequest d;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		d.docid = v.get<std::string>( "docid", "" );	

		return d;	
	}
	
	static void set( value &v, DocumentDeleteRequest const &d )
	{
		v.set( "docid", d.docid );
	}
};

template<>
struct traits<DocumentGetRequest> {

	static DocumentGetRequest get( value const &v )
	{
		DocumentGetRequest d;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		// TODO: should we also accept int, float? how can we fall back?
		d.docid = v.get<std::string>( "docid", "" );		
		
		return d;
	}
	
	static void set( value &v, DocumentGetRequest const &d )
	{
		v.set( "docid", d.docid );
	}
};

template<>
struct traits<DocumentGetAnswer> {
	
	static DocumentGetAnswer get( value const &v )
	{
		DocumentGetAnswer a;
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		a.docno = v.get<strus::Index>( "docno" );
		a.attributes = v.get<std::vector<std::pair<std::string, std::string> > >( "attributes" );
		a.metadata = v.get<std::vector<std::pair<std::string, strus::NumericVariant> > >( "metadata" );
		a.forward = v.get<std::vector<boost::tuple<std::string, std::string, strus::Index> > >( "forward" );
		a.search = v.get<std::vector<boost::tuple<std::string, std::string, strus::Index> > >( "search" );

		return a;
	}
	
	static void set( value &v, DocumentGetAnswer const &a )
	{
		v.set( "docno", a.docno );
		v.set( "attributes", a.attributes );
		v.set( "metadata", a.metadata );
		v.set( "forward", a.forward );
		v.set( "search", a.search );
	}
};	

template<>
struct traits<std::pair<std::string, strus::NumericVariant> > {
	
	static std::pair<std::string, strus::NumericVariant> get( value const &v )
	{
		std::pair<std::string, strus::NumericVariant> p;
		
		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		p.first = v.get<std::string>( "key" );
		value val = v["value"];
		switch( val.type( ) ) {
			case is_boolean:
				// TODO: really? Do we allow this?
				p.second = ( v.get<bool>( "value" ) ) ? (uint64_t)1 : (uint64_t)0;
				break;
			
			case is_string:
				// TODO: really? Do we allow this?
			case is_number:
				if( is_of_type( val, p.second.variant.Int ) ) {
					p.second = v.get<BOOST_TYPEOF( p.second.variant.Int )>( "value" );
				} else if( is_of_type( val, p.second.variant.UInt ) ) {
					p.second = v.get<BOOST_TYPEOF( p.second.variant.UInt )>( "value" );
				} else if( is_of_type( val, p.second.variant.Float ) ) {
					p.second = v.get<BOOST_TYPEOF( p.second.variant.Float )>( "value" );
				} else {
					throw bad_value_cast( );
				}
				break;
				
			case is_undefined:
			case is_null:
				// TODO: how do we map absence
			case is_object:
			case is_array:	
			default:
				throw bad_value_cast( );
		}
				
		return p;
	}

	static void set( value &v, std::pair<std::string, strus::NumericVariant> const &p )
	{
		v.set( "key", p.first );
		switch( p.second.type ) {
			case strus::NumericVariant::Null:
				v.set( "value", cppcms::json::null( ) );
				break;
			
			case strus::NumericVariant::Int:
				v.set( "value", p.second.toint( ) );
				break;
				
			case strus::NumericVariant::UInt:
				v.set( "value", p.second.touint( ) );
				break;
				
			case strus::NumericVariant::Float:
				v.set( "value", p.second.tofloat( ) );
				break;
			
			default:
				throw bad_value_cast( );
		}
	}
};

template<>
struct traits<boost::tuple<std::string, std::string, strus::Index> > {
	
	static boost::tuple<std::string, std::string, strus::Index> get( value const &v )
	{
		boost::tuple<std::string, std::string, strus::Index> t;

		if( v.type( ) != is_object) {
			throw bad_value_cast( );
		}
		
		t = boost::make_tuple(
			v.get<std::string>( "type" ),
			v.get<std::string>( "value" ),
			v.get<strus::Index>( "pos" )
		);
				
		return t;
	}
	
	static void set( value &v, boost::tuple<std::string, std::string, strus::Index> const &t )
	{
		v.set( "type", boost::get<0>( t ) );
		v.set( "value", boost::get<1>( t ) );
		v.set( "pos", boost::get<2>( t ) );
	}
};

} } // namespace cppcms::json

#endif

