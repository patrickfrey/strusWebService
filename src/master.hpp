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
		void register_common_pages( );

	protected:
		void report_error( unsigned int code, const std::string &msg );
		void not_found_404( );
};

} // namespace apps

#endif
