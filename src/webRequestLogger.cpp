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
#include "webRequestLogger.hpp"
#include "internationalization.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/utf8.hpp"
#include <cstdarg>
#include <cstring>
#include <fstream>

using namespace strus;

static int nofslotspow( int nofslots)
{
	int ii=1;
	int xx=10;
	for (; xx <= nofslots; xx *= 10, ++ii){}
	return ii;
}

static std::string getLogFilename( const std::string& logfilename, int procid, int nofprocs)
{
	if (nofprocs > 1)
	{
		char procnostr[ 32];
		std::snprintf( procnostr, sizeof(procnostr), "-%0*d", nofslotspow(nofprocs), procid);
		std::string ext;
		int ec = strus::getFileExtension( logfilename, ext);
		if (ec) throw strus::runtime_error( _TXT("failed to get file extension: %d"), ec);
		std::string rt( logfilename.c_str(), logfilename.size()-ext.size());
		rt.append( procnostr);
		rt.append( ext);
		return rt;
	}
	else
	{
		return logfilename;
	}
}

WebRequestLogger::WebRequestLogger( const std::string& logfilename_, int verbosity, int mask_, int structDepth_, int maxnofthreads, int procid_, int nofprocs_)
	:m_slots(new Slot[ maxnofthreads])
	,m_nofslots(maxnofthreads?maxnofthreads:1)
	,m_nofslotspow(nofslotspow(maxnofthreads))
	,m_logfilename(getLogFilename(logfilename_,procid_,nofprocs_))
	,m_logfile()
	,m_logout(0)
	,m_verbose(verbosity>0)
	,m_structDepth(structDepth_)
	,m_mask((WebRequestLoggerInterface::Mask)mask_)
	,m_gotAlert(false)
	,m_procid(procid_)
	,m_nofprocs(nofprocs_)
{
	if (m_logfilename == "stderr" || m_logfilename == "/dev/stderr")
	{
		m_logout = &std::cerr;
	}
	else if (m_logfilename == "-" || m_logfilename == "stdout" || m_logfilename == "/dev/stdout")
	{
		m_logout = &std::cout;
	}
	else
	{
		m_logfile.open( m_logfilename.c_str(), std::ofstream::out | std::ofstream::app);
		m_logout = &m_logfile;
	}
	if (verbosity)
	{
		int mm = m_mask;
		switch (verbosity)
		{
			case 4: mm |= (LogContentEvents|LogConnectionEvents);
			case 3: mm |= (LogMethodCalls|LogAction);
			case 2: mm |= (LogRequests|LogDelegateRequests|LogConfiguration);
			case 1: mm |= (LogError|LogWarning);
			case 0: mm |= (LogNothing); break;
			default: mm = LogAll;
		}
		m_mask = (WebRequestLoggerInterface::Mask)mm;
	}
	for (std::size_t si=0; si<m_nofslots; ++si)
	{
		m_slots[ si].init_id( m_nofslotspow, si);
	}
	reset();
}

WebRequestLogger::~WebRequestLogger()
{
	if (m_logout == &m_logfile)
	{
		m_logfile.close();
	}
	delete [] m_slots;
}

void WebRequestLogger::logRequest( const char* reqstr)
{
	logMessage( _TXT("request: {%s}"), reqstr);
}

void WebRequestLogger::logDelegateRequest( const char* address, const char* method, const char* content)
{
	std::size_t contentsize = std::strlen( content);
	if (contentsize == 0)
	{
		logMessage( _TXT("delegate %s '%s'"), method, address);
	}
	else if (contentsize <= 100)
	{
		logMessage( _TXT("delegate %s '%s': '%s'"), method, address, content);
	}
	else
	{
		const char* contentend = strus::utf8prev( content + 100);
		if (contentend < content) contentend = content;
		std::string contentstr( content, contentend - content);

		logMessage( _TXT("delegate %s '%s': '%s...'"), method, address, contentstr.c_str());
	}
}

void WebRequestLogger::logPutConfiguration( const char* type, const char* name, const std::string& configstr)
{
	logMessage( _TXT("configuration %s '%s': {%s}"), type, name, configstr.c_str());
}

void WebRequestLogger::logAction( const char* type, const char* name, const char* action)
{
	logMessage( _TXT("do %s %s '%s'"), action, type, name);
}

void WebRequestLogger::logContentEvent( const char* title, const char* item, const char* value)
{
	if (value)
	{
		if (item && item[0])
		{
			logMessage( _TXT("event %s %s '%s'"), title, item, value);
		}
		else
		{
			logMessage( _TXT("event %s '%s'"), title, value);
		}
	}
	else
	{
		if (item && item[0])
		{
			logMessage( _TXT("event %s %s"), title, item);
		}
		else
		{
			logMessage( _TXT("event %s"), title);
		}
	}
}

void WebRequestLogger::logConnectionEvent( const char* content)
{
	logMessage( _TXT("curl %s"), content);
}

void WebRequestLogger::logMethodCall(
		const char* classname,
		const char* methodname,
		const char* arguments,
		const char* result,
		const char* resultvar)
{
	if (!methodname || !methodname[0])
	{
		if (!classname || !classname[0])
		{
			logMessage( _TXT("assign %s := %s"), resultvar, result);
		}
		else if (!resultvar || !resultvar[0])
		{
			logMessage( _TXT("new %s( %s)"), classname, arguments);
		}
		else
		{
			logMessage( _TXT("new %s := %s( %s)"), classname, resultvar, arguments);
		}
	}
	else if (!result || !result[0])
	{
		logMessage( _TXT("call %s::%s( %s)"), classname, methodname, arguments);
	}
	else
	{
		if (!resultvar || !resultvar[0])
		{
			logMessage( _TXT("call %s::%s( %s) returns %s := %s"), classname, methodname, arguments, resultvar, result);
		}
		else
		{
			logMessage( _TXT("call %s::%s( %s) returns %s"), classname, methodname, arguments, result);
		}
	}
}

void WebRequestLogger::logError( const char* errmsg)
{
	logMessage( _TXT("error: %s"), errmsg);
}

void WebRequestLogger::logWarning( const char* errmsg)
{
	logMessage( _TXT("warning: %s"), errmsg);
}

int WebRequestLogger::logMask() const
{
	return m_mask;
}

void WebRequestLogger::reset()
{
	std::size_t ti;
	for (ti=0; ti<m_nofslots; ++ti)
	{
		m_slots[ti].reset();
	}
	if (m_logout == &m_logfile)
	{
		m_logfile.close();
		m_logfile.open( m_logfilename.c_str(), std::ofstream::out | std::ofstream::app);
	}
}

void WebRequestLogger::logMessageBuf( char* buf, size_t bufsize, const char* fmt, va_list ap)
{
	std::size_t buflen = std::snprintf( buf, bufsize, "#%s ", getThreadId());
	buflen += std::vsnprintf( buf+buflen, bufsize-buflen, fmt, ap);
	if (bufsize-4 <= buflen)
	{
		std::memcpy( buf+bufsize-4, "...", 4);
		buflen = bufsize-1;
	}
	char* bi = buf;
	char* ci = buf;
	for (; *bi; ++bi,++ci)
	{
		if ((unsigned char)*bi <= 32)
		{
			for (; *bi && (unsigned char)*bi <= 32; ++bi){}
			if (!*bi) break;
			*ci++ = ' ';
			*ci = *bi;
		}
		else
		{
			*ci = *bi;
		}
	}
	*ci = '\0';
}

void WebRequestLogger::alert( const char* msg)
{
	std::cerr << "Failed to log: " << msg << std::endl;
	m_gotAlert = true;
}

void WebRequestLogger::logMessage( const char* fmt, ...)
{
	char buf[ 4096];
	va_list ap;
	va_start(ap, fmt);
	logMessageBuf( buf, sizeof(buf), fmt, ap);
	try
	{
		strus::unique_lock lock( m_mutex);
		(*m_logout) << buf << std::endl;
		if (m_verbose && m_logout != &std::cerr && m_logout != &std::cout)
		{
			std::cerr << buf << std::endl;
		}
	}
	catch (const std::exception& err)
	{
		alert( err.what());
	}
	va_end(ap);
}

const char* WebRequestLogger::getThreadId()
{
	strus::ThreadId::Type tid = strus::ThreadId::get();
	std::size_t ti;
	for (ti=0; ti<m_nofslots; ++ti)
	{
		if (m_slots[ti].flag.test() && m_slots[ti].id == tid) return m_slots[ti].idstr;
	}
	if (ti == m_nofslots)
	{
		// ... thread has no context yet occupied. We occupy one
		for (ti=0; ti<m_nofslots; ++ti)
		{
			if (m_slots[ti].flag.set( true)) break;
		}
		if (ti == m_nofslots)
		{
			return "OVFL";
		}
		m_slots[ti].id = tid;
	}
	return m_slots[ti].idstr;
}

void WebRequestLogger::Slot::init_id( int idxpow, int idx)
{
	std::snprintf( idstr, sizeof(idstr), "%0*d", idxpow, idx);
}

void WebRequestLogger::Slot::reset()
{
	id = Id();
	flag.set( false);
}


