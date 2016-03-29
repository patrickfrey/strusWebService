/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
