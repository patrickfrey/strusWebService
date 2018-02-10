#include <cppcms/application.h>  
#include <cppcms/applications_pool.h>  
#include <cppcms/service.h>  
#include <cppcms/http_response.h>  
#include <iostream>

class hello : public cppcms::application {  
public:  
	hello(cppcms::service &srv)
		:cppcms::application(srv)
	{
	}
	virtual void main(std::string url);
};

void hello::main(std::string url_)
{
	std::cerr << "request " << url_ << std::endl;
	response().out() << "<html>\n<body>\n<h1>Hello World</h1>\n</body>\n</html>\n";
}

int main(int argc,char ** argv)  
{
	try {
		cppcms::service srv(argc,argv);
		srv.applications_pool().mount( cppcms::applications_factory<hello>());

		srv.run();
	}
	catch(std::exception const &e) {
		std::cerr << e.what() << std::endl;
	}
}


