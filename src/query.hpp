#ifndef QUERY_HPP
#define QUERY_HPP

#include "master.hpp"

namespace apps {

class query : public master {

	public:
		query( strusWebService &service );
		
	private:
		void query_cmd( const std::string name );
};

} // namespace apps

#endif

