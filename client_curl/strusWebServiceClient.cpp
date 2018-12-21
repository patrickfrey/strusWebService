#include "strus/lib/webrequest.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/numstring.hpp"
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <curl/curl.h>

static bool g_verbose = false;
static int g_errorsAccepted = -1;

static void printUsage()
{
	std::cerr << "strusWebServiceClient [-V] [-h] [-A <http-accept>] <method> <url> <parameter>" << std::endl;
}

template <typename DATA>
static CURLcode set_curl_opt( CURL *curl, CURLoption opt, const DATA& data)
{
	return curl_easy_setopt( curl, opt, data);
}

static void set_http_header( struct curl_slist*& headers, const char* name, const std::string& value)
{
	char buf[1024];
	if ((int)sizeof(buf) < std::snprintf( buf, sizeof(buf), "%s: %s", name, value.c_str())) throw std::bad_alloc();
	headers = curl_slist_append( headers, buf);
	if (g_verbose) std::cerr << "* " << buf << std::endl;
}

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t nn = size*nmemb;
	std::string* output = (std::string*)userdata;
	output->append( (char*)ptr, nn);
	return nn;
}



struct Request
{
	CURLcode errcode;
	std::string method;
	std::string content;
	std::string content_type;
	std::string user_agent;
	std::string accept;
	std::string accept_charset;

	Request( const char* method_, const std::string& content_, const char* doctype, const char* encoding, const char* http_accept)
		:errcode(CURLE_OK),method(method_),content(content_),content_type(),user_agent(),accept(),accept_charset()
	{
		user_agent = strus::string_format( "libcurl/%s",curl_version_info(CURLVERSION_NOW)->version);
		std::transform( method.begin(), method.end(), method.begin(), ::toupper);
		content_type = strus::string_format( "%s; charset=%s", doctype, encoding);
		accept = http_accept ? http_accept : content_type;
		accept_charset = encoding;
		if (content_type.empty()) throw std::bad_alloc();
	}

	struct Response
	{
		int httpstatus;
		char errbuf[ CURL_ERROR_SIZE];
		std::string content;

		Response()
		{
			errbuf[0] = '\0';
			httpstatus = 0;
		}
	};

	/// \brief Issue the request, return the status code
	CURLcode issue( const std::string& url, int port, Response& response, std::ostream& errout) const
	{
		CURL *curl = 0;
		CURLcode rt = CURLE_OK;
		struct curl_slist* headers = 0;

		curl = curl_easy_init();
		if (curl) try
		{
			set_curl_opt( curl, CURLOPT_USERAGENT, user_agent.c_str());

			set_http_header( headers, "Expect", "");
			set_http_header( headers, "Content-Type", content_type);
			set_http_header( headers, "Accept", accept);
			set_http_header( headers, "Accept-Charset", accept_charset);
			set_curl_opt( curl, CURLOPT_POST, 1);
			set_curl_opt( curl, CURLOPT_HTTPHEADER, headers);
			set_curl_opt( curl, CURLOPT_POSTFIELDSIZE, content.size());
			set_curl_opt( curl, CURLOPT_POSTFIELDS, content.c_str());
			std::string output;
			set_curl_opt( curl, CURLOPT_WRITEDATA, &response.content);
			set_curl_opt( curl, CURLOPT_WRITEFUNCTION, write_callback); 
			set_curl_opt( curl, CURLOPT_FAILONERROR, 1);
			set_curl_opt( curl, CURLOPT_ERRORBUFFER, response.errbuf);
			set_curl_opt( curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	
			set_curl_opt( curl, CURLOPT_URL, url.c_str());
			set_curl_opt( curl, CURLOPT_CUSTOMREQUEST, method.c_str());
			if (port) set_curl_opt( curl, CURLOPT_PORT, port);
			if (g_verbose) set_curl_opt( curl, CURLOPT_VERBOSE, 1);

			rt = curl_easy_perform( curl);
			if (rt == CURLE_OK)
			{
				long http_code = 0;
				curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code);
				response.httpstatus = http_code;
			}
			else
			{
				errout << curl_easy_strerror(rt) << std::endl;
				errout << response.errbuf << std::endl;
			}
		}
		catch (const std::bad_alloc& err)
		{
			rt = CURLE_OUT_OF_MEMORY;
		}
		if (headers) curl_slist_free_all( headers);
		if (curl) curl_easy_cleanup( curl);
		return rt;
	}
};

struct RequestInput
{
	std::vector<std::string> filelist;
	std::string content;
	const char* doctype;
	const char* encoding;
	const char* http_accept;

	RequestInput( const char* arg, const char* http_accept_)
		:doctype(0),encoding(0),http_accept(http_accept_)
	{
		if (arg[0] == '@')
		{
			int ec = strus::expandFilePattern( arg+1, filelist);
			if (ec) throw std::runtime_error( strus::string_format( "failed to read input files: %s", ::strerror(ec)));
			if (filelist.empty()) throw std::runtime_error( "invalid input file path");
			ec = strus::readFile( filelist[0], content);
			if (ec) throw std::runtime_error( strus::string_format( "failed to read input file %s: %s", filelist[0].c_str(), ::strerror(ec)));

			encoding = strus::guessCharsetEncoding( content.c_str(), content.size());
			doctype = strus::guessContentType( content.c_str(), content.size());

			if (!doctype) throw std::runtime_error( strus::string_format( "failed to determine content type of input file %s", filelist[0].c_str()));
			if (!encoding) throw std::runtime_error( strus::string_format( "failed to determine charset encoding of input file %s", filelist[0].c_str()));
		}
		else
		{
			doctype = http_accept ? http_accept : "text/plain";
			encoding = "UTF-8";
			content = arg;
		}
	}

	bool issueRequest( const Request& request, const std::string& url, int port, std::ostream& resout, std::ostream& errout)
	{
		bool rt = false;
		Request::Response response;
		CURLcode res = request.issue( url, port, response, errout);
		if (res == CURLE_OK)
		{
			if (response.httpstatus >= 200 && response.httpstatus <= 299)
			{
				if (g_verbose) errout << "Response code: " << response.httpstatus << std::endl;
				rt = true;
			}
			else
			{
				errout << "Response code: " << response.httpstatus << std::endl;
			}
			if (!response.content.empty())
			{
				resout << response.content << std::endl << std::endl;
			}
		}
		else
		{
			errout << "failed to issue request: " << curl_easy_strerror(res) << std::endl;
		}
		return rt;
	}

	bool process( const char* method, const std::string& url, int port, std::ostream& resout, std::ostream& errout, int& nofRequestsProcessed, int& nofRequestsFailed)
	{
		nofRequestsProcessed = 0;
		nofRequestsFailed = 0;
		if (filelist.empty())
		{
			Request request( method, content, doctype, encoding, http_accept);
			if (request.errcode != CURLE_OK)
			{
				errout << "failed to create request: " << curl_easy_strerror(request.errcode) << std::endl;
				nofRequestsFailed = 1;
				return false;
			}
			Request::Response response;
			if (issueRequest( request, url, port, resout, errout))
			{
				nofRequestsProcessed = 1;
				return true;
			}
			else
			{
				nofRequestsFailed = 1;
				return false;
			}
		}
		else
		{
			bool rt = true;
			nofRequestsProcessed = 0;
			std::vector<std::string>::const_iterator fi = filelist.begin(), fe = filelist.end();
			for (; fi != fe; ++fi)
			{
				if (nofRequestsFailed > g_errorsAccepted) return false;

				std::string filecontent;
				int ec = strus::readFile( *fi, filecontent);
				if (ec)
				{
					errout << strus::string_format( "failed to read input file %s: %s", filelist[0].c_str(), ::strerror(ec)) << std::endl;
					nofRequestsFailed += 1;
					rt = false;
					continue;
				}
				Request request( method, filecontent, doctype, encoding, http_accept);
				if (request.errcode != CURLE_OK)
				{
					errout << "failed to create request: " << curl_easy_strerror(request.errcode) << std::endl;
					nofRequestsFailed += 1;
					rt = false;
					continue;
				}
				Request::Response response;
				if (issueRequest( request, url, port, resout, errout))
				{
					nofRequestsProcessed += 1;
				}
				else
				{
					nofRequestsFailed += 1;
					rt = false;
				}
			}
			return rt;
		}
	}
};

static int getPort( char const* url)
{
	char const* ri = std::strchr( url, ':');
	if (ri && ri[1] == '/') ri = std::strchr( ri+1, ':');
	return ri ? atoi( ri+1) : 0;
}

static std::string getUrl( char const* url)
{
	char const* ri = std::strchr( url, ':');
	if (ri && ri[1] == '/') ri = std::strchr( ri+1, ':');
	if (ri)
	{
		std::string rt;
		rt.append( url, ri-url);
		for (++ri; *ri >= '0' && *ri <= '9'; ++ri){}
		rt.append( ri);
		return rt;
	}
	else
	{
		return url;
	}
}

int main( int argc, char const* argv[])
{
	try
	{
		int rt = 0;
		int argi = 1;
		const char* http_accept = 0;

		for (; argi < argc && argv[argi][0] == '-'; ++argi)
		{
			char const* arg = argv[argi];
			if (0==std::strcmp( arg, "--")) {++argi; break;}
			else if (0==std::strcmp( arg, "-h") || 0==std::strcmp( arg, "--help")) {printUsage(); exit(0);}
			else if (0==std::strcmp( arg, "-A") || 0==std::strcmp( arg, "--accept")) {http_accept=argv[++argi]; if (!http_accept) throw std::runtime_error("option -A expects argument");}
			else if (0==std::strcmp( arg, "-V") || 0==std::strcmp( arg, "--verbose")) {g_verbose = true;}
			else if (0==std::strcmp( arg, "-E") || 0==std::strcmp( arg, "--errors")) {g_errorsAccepted=strus::numstring_conv::touint( argv[++argi], std::numeric_limits<int>::max());}
			else throw std::runtime_error("unknown option");
		}
		if (argc-argi < 2)
		{
			printUsage();
			exit(-1);
		}
		RequestInput input( argc-argi < 3 ? "" : argv[argi+2]/*arg*/, http_accept);
		std::string url = getUrl( argv[argi+1]);
		int port = getPort( argv[argi+1]);
		const char* method = argv[argi]/*method*/;
		int nofRequestsProcessed = 0;
		int nofRequestsFailed = 0;

		curl_global_init( CURL_GLOBAL_ALL);
		if (!input.process( method, url, port, std::cout, std::cerr, nofRequestsProcessed, nofRequestsFailed))
		{
			std::cerr << strus::string_format( "%d out of %d requests failed", nofRequestsFailed, (nofRequestsProcessed + nofRequestsFailed)) << std::endl;
			rt = -2;
		}
		else
		{
			std::cerr << strus::string_format( "OK %d", nofRequestsProcessed) << std::endl;
		}
		curl_global_cleanup();
		return rt;
	}
	catch (const std::exception& err)
	{
		std::cerr << "ERROR " << err.what() << std::endl;
		return -1;
	}
}

