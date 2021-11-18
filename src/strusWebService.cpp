/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 * Copyright (c) 2017,2018 Patrick P. Frey, Andreas Baumann, Eurospider IT AG Zurich
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/lib/error.hpp"
#include "strus/lib/module.hpp"
#include "strus/webRequestHandlerInterface.hpp"
#include "strus/webRequestContextInterface.hpp"
#include "strus/webRequestLoggerInterface.hpp"
#include "strus/moduleLoaderInterface.hpp"
#include "strus/errorCodes.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "webRequestLogger.hpp"
#include "defaultContants.hpp"
#include "configUtils.hpp"
#include "applicationImpl.hpp"
#include "serviceClosure.hpp"
#include "versionWebService.hpp"
#include "strus/versionStorage.hpp"
#include "strus/versionModule.hpp"
#include "strus/versionRpc.hpp"
#include "strus/versionTrace.hpp"
#include "strus/versionAnalyzer.hpp"
#include "strus/versionBase.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/shared_ptr.hpp"
#include "strus/base/programOptions.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/platform.hpp"
#include <booster/log.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace strus::webservice;

#define WEBSERVICE_LIBRARY "cppcms"

// Global flags:
static int g_verbosity = 0;
static bool g_normal_termination = true;
strus::shared_ptr<ServiceClosure> g_serviceClosure;

static strus::AtomicFlag g_terminate;
static strus::AtomicFlag g_got_sighup;

static void callService( const char* methodname, void (ServiceClosure::*method)())
{
	strus::shared_ptr<ServiceClosure> sc( g_serviceClosure);
	if (sc.get())
	{
		try
		{
			((*sc).*method)();
		}
		catch( const std::exception& err)
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << _TXT("got exception on service shutdown: ") << err.what();
			std::cerr << _TXT("service method call terminated with exception") << " (" << methodname << "): " << err.what();
			g_terminate.set( true);
		}
	}
}

#define CALL_SERVICE( MT) callService( #MT, &ServiceClosure::MT);

// Signal handler:
static void signal_handler( int sig )
{
	if (g_verbosity)
	{
		std::cerr << "got signal " << sig << std::endl;
	}
	switch( sig ) {
		case SIGHUP:
		{
			if (g_got_sighup.set( true))
			{
				CALL_SERVICE( shutdown)
			}
			break;
		}
		default:
			// unknown signal, ignore
			break;
	}
}

static void exit_handler()
{
	if (!g_normal_termination)
	{
		std::cerr << strus::string_format( _TXT("exit called by %s (abrupt termination"), WEBSERVICE_LIBRARY) << std::endl;
		BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << _TXT("abrupt service termination, call of exit()");
	}
}

static void print3rdPartyLicenses( const cppcms::json::value& config, strus::ErrorBufferInterface* errorhnd)
{
	strus::local_ptr<strus::ModuleLoaderInterface> moduleLoader( strus::createModuleLoader( errorhnd));
	if (!moduleLoader.get()) throw std::runtime_error( _TXT("failed to create module loader"));
	std::string modpath = config.get( "extensions.directory", std::string());
	if (!modpath.empty())
	{
		moduleLoader->addModulePath( modpath);
		moduleLoader->addSystemModulePath();
	}
	std::vector<std::string> modules = getConfigArray( config, "extensions.modules");
	std::vector<std::string>::const_iterator mi = modules.begin(), me = modules.end();
	for (; mi != me; ++mi)
	{
		if (!moduleLoader->loadModule( *mi)) throw strus::runtime_error(_TXT("failed to load module %s"), mi->c_str());
	}
	if (errorhnd->hasError())
	{
		throw strus::runtime_error(_TXT("failed to load modules"));
	}
	std::vector<std::string> licenses_3rdParty = moduleLoader->get3rdPartyLicenseTexts();
	std::vector<std::string>::const_iterator ti = licenses_3rdParty.begin(), te = licenses_3rdParty.end();
	if (ti != te) std::cout << _TXT("3rd party licenses:") << std::endl;
	for (; ti != te; ++ti)
	{
		std::cout << *ti << std::endl;
	}
	std::cout << std::endl;
}

static void printVersion()
{
	std::cout << DefaultConstants::PACKAGE() << std::endl;
	std::cout << _TXT("Strus webservice version ") << STRUS_WEBSERVICE_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus module version ") << STRUS_MODULE_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus rpc version ") << STRUS_RPC_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus trace version ") << STRUS_TRACE_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus analyzer version ") << STRUS_ANALYZER_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus storage version ") << STRUS_STORAGE_VERSION_STRING << std::endl;
	std::cout << _TXT("Strus base version ") << STRUS_BASE_VERSION_STRING << std::endl;
}

static void printUsage()
{
	std::cout << _TXT("usage:") << " strusWebservice [options]" << std::endl;
	std::cout << _TXT("description: Run the strus web service.") << std::endl;
	std::cout << _TXT("options:") << std::endl;
	std::cout << "-h|--help" << std::endl;
	std::cout << "    " << _TXT("Print this usage and do nothing else") << std::endl;
	std::cout << "-v|--version" << std::endl;
	std::cout << "    " << _TXT("Print the program version and do nothing else") << std::endl;
	std::cout << "--license" << std::endl;
	std::cout << "    " << _TXT("Print 3rd party licences requiring reference") << std::endl;
	std::cout << "-c|--config <CONFIG>" << std::endl;
	std::cout << "    " << _TXT("Define the web service configuration file as <CONFIG>") << std::endl;
	std::cout << "-P|--port <PORT>" << std::endl;
	std::cout << "    " << _TXT("Define the port the service is listening on, overwrite") << std::endl;
	std::cout << "    " << _TXT("  the configured value service.port with <PORT>.") << std::endl;
	std::cout << "-N|--name <SERVICENAME>" << std::endl;
	std::cout << "    " << _TXT("Define the name of the service, overwrite") << std::endl;
	std::cout << "    " << _TXT("  the configured value service.name with <SERVICENAME>.") << std::endl;
	std::cout << "--cleanup" << std::endl;
	std::cout << "    " << _TXT("Do cleanup of configuration before startup.") << std::endl;
	std::cout << "--nostart" << std::endl;
	std::cout << "    " << _TXT("Do not run the service but terminate after initialization.") << std::endl;
	std::cout << "-V|--verbose" << std::endl;
	std::cout << "    " << _TXT("Do increase verbosity level for logging and output.") << std::endl;
	std::cout << "    " << _TXT("  may be repeated, e.g. -VVV for verbosity level 3.") << std::endl;
	std::cout << "--verbosity <N>" << std::endl;
	std::cout << "    " << _TXT("Do increase verbosity level for logging and output by <N>.") << std::endl;
	std::cout << "    " << _TXT("  This is a synonym for option -V,--verbose, another way to specify") << std::endl;
	std::cout << "    " << _TXT("  -V,-VV,-VVV as a number") << std::endl;
}

static std::string beautifyErrorMessage( const std::string& msg)
{
	std::string rt;
	char const* si = msg.c_str();
	char const* start;
	char eb;
	int cnt;
	while (*si)
	{
		switch (*si)
		{
			case '"':
			case '\'':
				for (eb=*si,start=si++; *si && *si != eb; ++si) if (*si == '\\' && si[1]) ++si;
				if (*si) ++si;
				rt.append( start, si - start);
				break;
			case ':':
				rt.push_back(*si++);
				if (*si == ' ')
				{
					rt.push_back('\n');
					rt.push_back(' ');
					while (*si == ' ') ++si;
				}
				break;
			case '(':
				for (cnt=0,eb=')',start=si++; *si && *si != eb; ++si) if (*si == '[' || *si == '{') cnt++;
				if (*si == eb)
				{
					if (cnt)
					{
						rt.push_back('\n');
						rt.push_back('\n');
					}
					++si;
					rt.append( start, si - start);
				}
				else
				{
					si = start;
					rt.push_back(*si++);
				}
				break;
			default:
				rt.push_back(*si++);
				break;
		 }
	}
	return rt;
}

int main( int argc_, const char *argv_[] )
{
	int rt = 0;
	// Select configured debug traces:
	strus::DebugTraceInterface* dbgtrace = strus::createDebugTrace_standard( 2/*threads*/);
	if (!dbgtrace)
	{
		std::cerr << _TXT("failed to create debug trace handler") << std::endl;
		return -1;
	}
	strus::local_ptr<strus::ErrorBufferInterface> errorhnd( strus::createErrorBuffer_standard( 0/*log file handle*/, 2/*nof threads*/, dbgtrace));
	if (!errorhnd.get())
	{
		std::cerr << _TXT("failed to create error handler") << std::endl;
		return -1;
	}
	try
	{
		// Inject error buffer interface as singleton for bindings context:
		strus::declareErrorBuffer_singleton( errorhnd.get());

		// Install exit handler to catch and report abrupt termination by cppcms
		int ec = std::atexit( exit_handler);

		// Define configuration and usage:
		strus::ProgramOptions opt(
				errorhnd.get(), argc_, argv_, 11,
				"h,help", "v,version", "X,schema:", "c,config:",
				"P,port:", "N,name:",
				"verbosity:", "V,verbose+", "license",
				"nostart", "cleanup");
		if (errorhnd->hasError())
		{
			throw strus::runtime_error(_TXT("failed to parse program arguments"));
		}
		bool printUsageAndExit = opt("help");

		if (!printUsageAndExit)
		{
			if (opt.nofargs() > 0)
			{
				std::cerr << _TXT("no arguments, only options expected") << std::endl;
				printUsageAndExit = true;
				rt = strus::ErrorCodeInvalidArgument;
			}
		}
		std::string configdir;
		std::string configfile;
		std::string servicename;
		int port = 0;
		bool doStart = true;
		bool doCleanup = false;

		if (opt("config"))
		{
			configfile = opt["config"];
			ec = strus::getParentPath( configfile, configdir);
			if (ec) throw strus::runtime_error(_TXT("failed to get parent path of configuration: %s"), ::strerror(ec));
		}
		if (opt("verbosity"))
		{
			g_verbosity += opt.asInt("verbosity");
		}
		if (opt("verbose"))
		{
			g_verbosity += opt.asInt("verbose");
		}
		cppcms::json::value config;
		if (!configfile.empty())
		{
			config = configFromFile( configfile);
		}
		else
		{
			config = configDefault();
		}
		if (opt("license"))
		{
			print3rdPartyLicenses( config, errorhnd.get());
		}
		if (opt("nostart"))
		{
			doStart = false;
		}
		if (opt("cleanup"))
		{
			doCleanup = true;
		}
		if (opt("license"))
		{
			print3rdPartyLicenses( config, errorhnd.get());
		}
		if (opt( "version"))
		{
			printVersion();
			return rt;
		}
		if (opt("port"))
		{
			port = opt.asInt( "port");
			if (port == 0) throw strus::runtime_error(_TXT("bad value for -P/--port configured"));
		}
		if (opt("name"))
		{
			servicename = opt["name"];
		}
		if (printUsageAndExit)
		{
			printUsage();
			return rt;
		}
		g_normal_termination = false;

		// Create logging directory if it not exists:
		std::string requestLogDir = config.get( "logging.directory", DefaultConstants::LOGGING_DIR());
		if (!requestLogDir.empty())
		{
			ec = strus::mkdirp( requestLogDir);
			if (ec) throw strus::runtime_error( _TXT("failed to create logging directory %s: %s"), requestLogDir.c_str(), ::strerror(ec));
		}

		// Install signal handlers
		signal( SIGHUP, signal_handler );

		// Implicit evaluation of number of threads to use if not explicitely defined:
		int nofCores = strus::platform::cores();
		if (nofCores <= 0) nofCores = 4;
		int nofThreads = config.get( "service.worker_threads", nofCores);
		if (config.find( "context.rpc").is_undefined())
		{
			// Overwrite number of threads configured as it is defined by cppcms
			config.set( "context.threads", nofThreads);
			config.set( "service.worker_threads", nofThreads);
			if (config.find( "service.applications_pool_size").is_undefined())
			{
				config.set( "service.applications_pool_size", nofThreads);
			}
		}
		// Overwrite logging level with verbosity defined on command line:
		if (g_verbosity)
		{
			config.set( "logging.level", "debug");
			std::cerr << "service configuration:" << std::endl << "---" << config.save( cppcms::json::readable) << std::endl << "---" << std::endl;
		}
		// Run the webservice:
		while( !g_terminate.test()) try
		{
			booster::log::logger::instance( ).remove_all_sinks();
			if (g_verbosity && config.get( "logging.stderr", false) == false)
			{
				booster::shared_ptr<booster::log::sinks::standard_error> csink( new booster::log::sinks::standard_error());
				booster::log::logger::instance().add_sink(csink);
			}
			if (port)
			{
				config.set( "service.port", port);
			}
			else
			{
				port = config.get( "service.port", 80);
			}
			if (!servicename.empty())
			{
				config.set( "service.name", servicename);
			}
			else
			{
				servicename = config.get( "service.name", DefaultConstants::DEFAULT_SERVICE_NAME());
			}
			g_serviceClosure.reset(); //... free old instance before creating new one
			g_serviceClosure.reset( new ServiceClosure( configdir));

			BOOSTER_INFO( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("initializing web service %s"), servicename.c_str());
			g_serviceClosure->init( config, g_verbosity);
			if (doCleanup)
			{
				BOOSTER_INFO( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("configuration cleanup %s"), servicename.c_str());
				g_serviceClosure->cleanupConfig();
			}
			if (!doStart) break;

			BOOSTER_INFO( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("starting strus web service (%d threads).."), nofThreads);
			g_serviceClosure->synchronize();

			CALL_SERVICE( mount_applications)
			BOOSTER_NOTICE( DefaultConstants::PACKAGE())
					<< strus::string_format( _TXT("service ready and listening on %d"), port);

			g_got_sighup.set( false);
			CALL_SERVICE( run);
			if (g_got_sighup.test())
			{
				BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("reloading configuration on SIGHUP..");
			} else if (g_terminate.set( true)) {
				BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("received shutdown command..");
				CALL_SERVICE( shutdown);
			}
		}
		catch (const std::exception& err)
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << _TXT("got exception in service event loop: ") << err.what();
			rt = strus::ErrorCodeUncaughtException;
			g_terminate.set( true);
		}
		if (doStart)
		{
			BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("service terminated");
		}
		else
		{
			BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("service not started (--nostart), done.");
		}
	}
	catch (const cppcms::json::bad_value_cast& e)
	{
		std::cerr << _TXT("ERROR bad value in configuration: ") << e.what() << std::endl;
		if (!rt) rt = strus::ErrorCodeSyntax;
	}
	catch (const std::bad_alloc&)
	{
		std::cerr << _TXT("ERROR ") << _TXT("out of memory") << std::endl;
		if (!rt) rt = strus::ErrorCodeOutOfMem;
	}
	catch (const std::runtime_error& e)
	{
		const char* errormsg = errorhnd.get() ? errorhnd->fetchError() : NULL;
		if (errormsg)
		{
			char const* erroritr = errormsg;
			int ec = strus::errorCodeFromMessage( erroritr);
			while (ec >= 0)
			{
				if (ec) rt = ec;
				ec = strus::errorCodeFromMessage( erroritr);
			}
			std::cerr << _TXT("ERROR\n") << beautifyErrorMessage( e.what()) + ":\n " << errormsg << std::endl;
		}
		else
		{
			std::cerr << _TXT("ERROR\n") << beautifyErrorMessage( e.what()) << std::endl;
		}
		if (!rt) rt = strus::ErrorCodeRuntimeError;
	}
	catch (const std::exception& e)
	{
		std::cerr << _TXT("EXCEPTION\n") << beautifyErrorMessage( e.what()) << std::endl;
		if (!rt) rt = strus::ErrorCodeUncaughtException;
	}
	catch (...)
	{
		std::cerr << _TXT("EXCEPTION unknown") << std::endl;
		if (!rt) rt = strus::ErrorCodeUncaughtException;
	}
	g_normal_termination = true;
	if (g_serviceClosure.get())
	{
		g_serviceClosure->shutdown();
		g_serviceClosure.reset();
	}
	return rt;
}

