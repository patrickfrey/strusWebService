#ifndef STRUS_WEB_SERVICE_HPP
#define STRUS_WEB_SERVICE_HPP

#include <cppcms/application.h>  

namespace apps {

class strusWebService : public cppcms::application {

	public:
		strusWebService( cppcms::service &srv );
};

} // namespace apps

#endif
