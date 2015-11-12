#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	ping( *this ),
	index( *this )
{
	add( master );
	add( ping );
	add( index );
	
	master.register_common_pages( );
}

} // namespace apps
