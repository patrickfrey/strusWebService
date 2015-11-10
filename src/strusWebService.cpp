#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	ping( *this )
{
	add( master );
	add( ping );
	
	master.register_common_pages( );
}

} // namespace apps
