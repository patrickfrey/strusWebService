#ifndef MASTER_HPP
#define MASTER_HPP

#include <cppcms/application.h>

namespace apps {

class strusWebService;

class master : public cppcms::application {
	
	protected:
		strusWebService &service;
		
	public:
		master( strusWebService &service );
};

} // namespace apps

#endif
