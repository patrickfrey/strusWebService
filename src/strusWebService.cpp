#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	ping( *this ),
	storage( *this )
{
	add( master );
	add( ping );
	add( storage );
	
	master.register_common_pages( );
}

} // namespace apps
