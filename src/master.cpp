#include "strusWebService.hpp"
#include "master.hpp"

#include <cppcms/service.h>

namespace apps {

master::master( strusWebService &service )
	: application( service.service( ) ),
	service( service )
{
}

} // namespace apps
