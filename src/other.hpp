#ifndef OTHER_HPP
#define OTHER_HPP

#include "master.hpp"

#include <cppcms/json.h>

namespace apps {

class other : public master {

	public:
		other( strusWebService &service );
		
	private:
		void ping_cmd( );
		void version_cmd( );
};

} // namespace apps

#endif
