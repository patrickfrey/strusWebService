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
#ifndef _STRUS_WEBREQUEST_LOGGER_HPP_INCLUDED
#define _STRUS_WEBREQUEST_LOGGER_HPP_INCLUDED
#include "strus/webRequestLoggerInterface.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/atomic.hpp"
#include "internationalization.hpp"
#include <iostream>
#include <fstream>

namespace strus
{

class WebRequestLogger
	:public strus::WebRequestLoggerInterface
{
public:
	WebRequestLogger( const std::string& logfilename_, int verbosity, int mask_, int structDepth, int maxnofthreads, int procid, int nofprocs);

	virtual ~WebRequestLogger();

	virtual int logMask() const;

	virtual int structDepth() const
	{
		return m_structDepth;
	}

	virtual void logRequest( const char* content, std::size_t contentsize);
	virtual void logRequestType( const char* title, const char* procdescr, const char* contextType, const char* contextName);
	virtual void logRequestAnswer( const char* content, std::size_t contentsize);

	virtual void logDelegateRequest( const char* address, const char* method, const char* content, std::size_t contentsize);
	virtual void logPutConfiguration( const char* contextType, const char* contextName, const std::string& configstr);
	virtual void logAction( const char* contextType, const char* contextName, const char* action);
	virtual void logContentEvent( const char* title, const char* item, const char* content, std::size_t contentsize);

	virtual void logMethodCall(
			const char* classname,
			const char* methodname,
			const char* arguments,
			const char* result,
			std::size_t resultsize,
			const char* resultvar);

	virtual void logConnectionEvent( const char* content);
	virtual void logConnectionState( const char* state, int arg);

	virtual void logWarning( const char* errmsg);
	virtual void logError( const char* errmsg);
	virtual void logContextInfoMessages( const char* content);

	void reset();

	enum {
		MaxLogContentSize=2048/*2K*/,
		MaxLogBinaryItemSize=32,
		MaxLogReadableItemSize=256
	};

private:
	void logMessageBuf( char* buf, size_t bufsize, const char* fmt, va_list ap);

	void logMessage( const char* fmt, ...)
#ifdef __GNUC__
	__attribute__ ((format (printf, 2, 3)))
#endif
	;
	const char* getThreadId();
	void alert( const char* msg);
	bool gotAlert() const
	{
		return m_gotAlert;
	}

private:
	strus::mutex m_mutex;
	struct Slot
	{
		typedef strus::ThreadId::Type Id;
		typedef strus::AtomicFlag Flag;

		Slot(){}
		~Slot(){}

		void init_id( int idxpow, int idx);
		void reset();

		Id id;
		Flag flag;
		char idstr[8];
	};
	Slot* m_slots;
	size_t m_nofslots;
	int m_nofslotspow;
	std::string m_logfilename;
	std::ofstream m_logfile;
	std::ostream* m_logout;
	bool m_verbose;
	int m_structDepth;
	Mask m_mask;
	bool m_gotAlert;
	int m_procid;
	int m_nofprocs;
};

}//namespace
#endif


