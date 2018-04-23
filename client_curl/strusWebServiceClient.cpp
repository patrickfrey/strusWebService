#include "strus/lib/webrequest.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <curl/curl.h>

static bool g_verbose = false;

static void printUsage()
{
	std::cerr << "strusWebServiceClient [-V] [-h] [-A <http-accept>] <method> <url> <parameter>" << std::endl;
}

struct Request
{
	int errcode;
	std::string method;
	std::string content;
	std::string content_type;
	std::string user_agent;
	std::string accept;
	std::string accept_charset;

	Request( const char* method_, const char* arg, const char* http_accept)
		:errcode(0),method(method_),content(),content_type(),user_agent(),accept(),accept_charset()
	{
		user_agent = strus::string_format( "libcurl/%s",curl_version_info(CURLVERSION_NOW)->version);
		std::transform( method.begin(), method.end(), method.begin(), ::toupper);
		if (arg[0] == '@')
		{
			errcode = strus::readFile( arg+1, content);
			if (!errcode)
			{
				const char* encoding = strus::guessCharsetEncoding( content.c_str(), content.size());
				const char* doctype = strus::guessContentType( content.c_str(), content.size());
				content_type = strus::string_format( "%s; charset=%s", doctype, encoding);
				accept = http_accept ? http_accept : content_type;
				accept_charset = encoding;
				if (content_type.empty()) throw std::bad_alloc();
				if (!encoding || !doctype) errcode = 22/*EINVAL*/;
			}
		}
		else
		{
			accept = http_accept ? http_accept : "text/plain";
			accept_charset = "UTF-8";
			content = arg;
		}
	}
};

static void set_http_header( struct curl_slist*& headers, const char* name, const std::string& value)
{
	char buf[1024];
	if ((int)sizeof(buf) < std::snprintf( buf, sizeof(buf), "%s: %s", name, value.c_str())) throw std::bad_alloc();
	headers = curl_slist_append( headers, buf);
	if (g_verbose) std::cerr << "* " << buf << std::endl;
}

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

template <typename DATA>
static void set_curl_opt( CURL *curl, CURLoption opt, const DATA& data)
{
	CURLcode res = curl_easy_setopt( curl, opt, data);
	if (res != CURLE_OK) throw res;
}

struct ContentBuf
{
	const char* ptr;
	std::size_t pos;
	std::size_t size;

	ContentBuf( const char* ptr_, std::size_t size_)
		:ptr(ptr_),pos(0),size(size_){}
};

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t nn = size*nmemb;
	std::string* output = (std::string*)userdata;
	output->append( (char*)ptr, nn);
	return nn;
}

int main( int argc, char const* argv[])
{
	int rt = 0;
	CURL *curl = 0;
	CURLcode res = CURLE_OK;
	struct curl_slist* headers = 0;
	char curl_errbuf[ CURL_ERROR_SIZE] = "";
	int argi = 1;
	const char* http_accept = 0;
	
	for (; argi < argc && argv[argi][0] == '-'; ++argi)
	{
		char const* arg = argv[argi];
		if (0==std::strcmp( arg, "--")) {++argi; break;}
		else if (0==std::strcmp( arg, "-h") || 0==std::strcmp( arg, "--help")) {printUsage(); exit(0);}
		else if (0==std::strcmp( arg, "-A") || 0==std::strcmp( arg, "--accept")) {http_accept=argv[++argi]; if (!http_accept) throw std::runtime_error("option -A expects argument");}
		else if (0==std::strcmp( arg, "-V") || 0==std::strcmp( arg, "--verbose")) {g_verbose = true;}
		else throw std::runtime_error("unknown option");
	}
	if (argc-argi < 2)
	{
		printUsage();
		exit(-1);
	}
	Request request( argv[argi]/*method*/, argc-argi < 3 ? "" : argv[argi+2]/*arg*/, http_accept);
	if (request.errcode)
	{
		std::cerr << std::strerror( request.errcode) << std::endl;
		return request.errcode;
	}
	ContentBuf cbuf( request.content.c_str(), request.content.size());
	std::string url = getUrl( argv[argi+1]);
	int port = getPort( argv[argi+1]);

	curl_global_init( CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) try
	{
		set_curl_opt( curl, CURLOPT_USERAGENT, request.user_agent.c_str());

		set_http_header( headers, "Expect", "");
		set_http_header( headers, "Content-Type", request.content_type);
		set_http_header( headers, "Accept", request.accept);
		set_http_header( headers, "Accept-Charset", request.accept_charset);
		set_curl_opt( curl, CURLOPT_POST, 1);
		set_curl_opt( curl, CURLOPT_HTTPHEADER, headers);
		set_curl_opt( curl, CURLOPT_POSTFIELDSIZE, request.content.size());
		set_curl_opt( curl, CURLOPT_POSTFIELDS, request.content.c_str());
		std::string output;
		set_curl_opt( curl, CURLOPT_WRITEDATA, &output);
		set_curl_opt( curl, CURLOPT_WRITEFUNCTION, write_callback); 
		//set_curl_opt( curl, CURLOPT_READDATA, &cbuf);
		//set_curl_opt( curl, CURLOPT_READFUNCTION, read_callback);
		set_curl_opt( curl, CURLOPT_FAILONERROR, 1);
		set_curl_opt( curl, CURLOPT_ERRORBUFFER, curl_errbuf);
		set_curl_opt( curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

		set_curl_opt( curl, CURLOPT_URL, url.c_str());
		set_curl_opt( curl, CURLOPT_CUSTOMREQUEST, request.method.c_str());
		if (port) set_curl_opt( curl, CURLOPT_PORT, port);
		if (g_verbose) set_curl_opt( curl, CURLOPT_VERBOSE, 1);

		res = curl_easy_perform( curl);
		if (res != CURLE_OK)
		{
			std::cerr << curl_easy_strerror(res) << std::endl;
			std::cerr << curl_errbuf << std::endl;
			rt = res;
		}
		else
		{
			long http_code = 0;
			curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (g_verbose) std::cerr << "\nResponse code: " << http_code << std::endl;
			std::cout << output << std::endl << std::endl;
		}
	}
	catch (const std::bad_alloc&)
	{
		int ec = 12/*ENOMEM*/;
		std::cerr << std::strerror( ec) << std::endl;
		rt = ec;
	}
	catch (const CURLcode& res_thrown)
	{
		std::cerr << curl_easy_strerror(res) << std::endl;
		rt = res_thrown;
	}
	if (headers) curl_slist_free_all( headers);
	if (curl) curl_easy_cleanup( curl);
	curl_global_cleanup();
	return rt;
}

