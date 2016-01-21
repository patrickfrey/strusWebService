/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdlib>
#include <cmath>

#include <boost/lexical_cast.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

#include <cppcms/json.h>

struct IntegralTypeTag;
struct FloatingPointTypeTag;

template<
	typename T,
	bool is_integral = boost::is_integral<T>::value,
	bool is_floating_point = boost::is_floating_point<T>::value
> struct TypeTag;

template<typename T>
struct TypeTag<T, true, false> {
	typedef IntegralTypeTag Type;
};

template<typename T>
struct TypeTag<T, false, true> {
	typedef FloatingPointTypeTag Type;
};

template<typename T, typename TypeTag = typename TypeTag<T>::Type> struct Types;

template<typename T>
struct Types<T, IntegralTypeTag> {
static bool is_of_type( cppcms::json::value const &v, T x )
{
	try {
		if( v.type( ) == cppcms::json::is_number ) {
			double n;
			double d = std::modf( v.number( ), &n );
			if( std::abs( d ) < std::numeric_limits<float>::epsilon( ) ) {
				// the fractional part is smaller than the machine eplsilon
				// of float (right?), so it is an integer
				return true;
			}
			return false;
		} else if( v.type( ) == cppcms::json::is_string ) {
			T t = boost::lexical_cast<T>( v.str( ) );
			(void)t;
			return true;
		} else {
			return false;
		}
	} catch( boost::bad_lexical_cast &e ) {
		return false;
	}
}

};

template <typename T>
struct Types<T, FloatingPointTypeTag> {
static bool is_of_type( cppcms::json::value const &v, T x )
{
	try {
		if( v.type( ) == cppcms::json::is_number ) {
			// we loose precision as ArithmenticType is float, but so what
			return true;
		} else if( v.type( ) == cppcms::json::is_string ) {
			T t = boost::lexical_cast<T>( v.str( ) );
			(void)t;
			return true;
		} else {
			return false;
		}
	} catch( boost::bad_lexical_cast &e ) {
		return false;
	}
}
};

template<typename T>
static bool is_of_type( cppcms::json::value const &v, T x )
{
	return Types<T>::is_of_type( v, x );
}

#endif
