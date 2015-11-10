#ifndef PING_HPP
#define PING_HPP

#include "master.hpp"

#include <cppcms/json.h>

namespace apps {

class ping : public master {

	public:
		ping( strusWebService &service );
		
	private:
		void ping_cmd( );
};

} // namespace apps

#endif
