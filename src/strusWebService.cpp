#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	other( *this ),
	index( *this, settings( ).get<std::string>( "storage.basedir" ) )
{
	add( master );
	add( other );
	add( index );
	
	master.register_common_pages( );
}

} // namespace apps
