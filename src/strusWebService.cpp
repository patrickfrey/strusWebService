/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 * Copyright (c) 2017,2018 Patrick P. Frey, Andreas Baumann, Eurospider IT AG Zurich
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
#include "strus/base/programOptions.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/string_format.hpp"
#include <booster/log.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#undef STRUS_LOWLEVEL_DEBUG
using namespace strus::webservice;

// Global flags:
static bool g_verbose = false;
static bool g_exit_silent = false;
static strus::AtomicFlag g_terminate;
static strus::AtomicFlag g_got_sighup;

// Signal handler:
static void signal_handler( int sig )
{
	if (g_verbose)
	{
		std::cerr << "got signal " << sig << std::endl;
	}
	switch( sig ) {
		case SIGHUP:
			g_got_sighup.set( true);
			break;
		default:
			// unknown signal, ignore
			break;
	}
}

static void on_exit_handler( int ec, void*)
{
	if (ec && !g_exit_silent)
	{
		const char* msg = NULL;
		if (ec == 226)
		{
			msg = _TXT("already used");
		}
		std::cerr << _TXT("exit called with code ") << ec;
		if (msg) std::cerr << " (" << msg << ")";
		std::cerr << std::endl;
	}
}

static void print3rdPartyLicenses( const cppcms::json::value& config, strus::ErrorBufferInterface* errorhnd)
{
	strus::local_ptr<strus::ModuleLoaderInterface> moduleLoader( strus::createModuleLoader( errorhnd));
	if (!moduleLoader.get()) throw strus::runtime_error( "%s", _TXT("failed to create module loader"));
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
	std::cout << "-V|--verbose" << std::endl;
	std::cout << "    " << _TXT("Do verbose logging and output") << std::endl;
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
	strus::DebugTraceInterface* dbgtrace = dbgtrace = strus::createDebugTrace_standard( 2/*threads*/);
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
	on_exit( on_exit_handler, NULL);
	try
	{
		// Define configuration and usage:
		strus::ProgramOptions opt(
				errorhnd.get(), argc_, argv_, 5,
				"h,help", "v,version", "c,config:", "V,verbose", "license");
		if (errorhnd->hasError())
		{
			throw strus::runtime_error(_TXT("failed to parse program arguments"));
		}
		bool printUsageAndExit = opt("help");

		if (opt( "version"))
		{
			printVersion();
		}
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
		if (opt("config"))
		{
			int ec = strus::getParentPath( opt["config"], configdir);
			if (ec) throw strus::runtime_error(_TXT("failed to get parent path of configuration: %s"), ::strerror(ec));
		}
		g_verbose = opt("verbose");
		cppcms::json::value config = opt("config") ? configFromFile( opt[ "config"], rt) : configDefault();

		if (opt("license"))
		{
			print3rdPartyLicenses( config, errorhnd.get());
		}
		if (printUsageAndExit)
		{
			printUsage();
			return rt;
		}

		// Install signal handlers
		signal( SIGHUP, signal_handler );

		ServiceClosure service( configdir, config, g_verbose);
		int nofThreads = service.threads_no();
		if (config.find( "context.rpc").is_undefined())
		{
			// Overwrite number of threads configured as it is defined by cppcms
			config.set( "context.threads", nofThreads);
		}
		if (g_verbose)
		{
			config.set( "logging.level", "debug");
			std::cerr << "service configuration:" << std::endl << "---" << config.save( cppcms::json::readable) << std::endl << "---" << std::endl;
		}
		// Run the webservice:
		while( !g_terminate.test()) try
		{
			booster::log::logger::instance( ).remove_all_sinks();

			service.init( config, g_verbose);
			if (g_verbose && config.get( "logging.stderr", false) == false)
			{
				booster::shared_ptr<booster::log::sinks::standard_error> csink( new booster::log::sinks::standard_error());
				booster::log::logger::instance().add_sink(csink);
			}
			BOOSTER_INFO( DefaultConstants::PACKAGE())
					<< strus::string_format(
							_TXT("starting strus web service (%d %s).."),
							nofThreads, nofThreads==1?"thread":"threads");
			service.mount_applications();

			service.run();
			if(g_got_sighup.test())
			{
				BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("reloading configuration on SIGHUP..");
				g_got_sighup.set( false);
			} else {
				BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("received shutdown command..");
				g_terminate.set( true);
			}

			try {
				service.shutdown();
			}
			catch( const std::exception& err)
			{
				BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << _TXT("got exception on service shutdown: ") << err.what();
				g_terminate.set( true);
			}
	
		}
		catch (const std::exception& err)
		{
			BOOSTER_ERROR( DefaultConstants::PACKAGE() ) << _TXT("got exception in service event loop: ") << err.what();
			rt = strus::ErrorCodeUncaughtException;
			g_terminate.set( true);
		}
		BOOSTER_INFO( DefaultConstants::PACKAGE() ) << _TXT("service terminated");
		return rt;
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
	g_exit_silent = true;
	return rt;
}

