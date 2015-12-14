#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdlib>
#include <cmath>

#include <boost/lexical_cast.hpp>
#include <boost/core/enable_if.hpp>
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
