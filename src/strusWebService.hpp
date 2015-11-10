#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "ping.hpp"

#include <cppcms/application.h>  

namespace apps {

class strusWebService : public cppcms::application {

	public:
		apps::master master;
		apps::ping ping;

	public:
		strusWebService( cppcms::service &srv );
};

} // namespace apps

#endif
