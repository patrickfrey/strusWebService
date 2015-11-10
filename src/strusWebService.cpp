#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	ping( *this )
{
	add( ping );
}

} // namespace apps
