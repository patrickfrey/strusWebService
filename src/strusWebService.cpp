#include "strusWebService.hpp"

namespace apps {

strusWebService::strusWebService( cppcms::service &srv )
	: cppcms::application( srv ),
	master( *this ),
	other( *this ),
	index( *this, settings( ).get<std::string>( "storage.basedir" ) ),
	document( *this ),
	query( *this )
{
	add( master );
	add( other );
	add( index );
	add( document );
	add( query );
	
	master.register_common_pages( );
}

} // namespace apps
