#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "master.hpp"

#include <cppcms/json.h>

namespace apps {

class storage : public master {

	public:
		storage( strusWebService &service );
		
	private:
		void create_cmd( const std::string name );
};

} // namespace apps

#endif
