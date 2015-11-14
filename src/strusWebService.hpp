#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include "master.hpp"
#include "other.hpp"
#include "index.hpp"

#include <cppcms/application.h>  

namespace apps {

class strusWebService : public cppcms::application {

	public:
		apps::master master;
		apps::other other;
		apps::index index;

	public:
		strusWebService( cppcms::service &srv );
};

} // namespace apps

#endif
