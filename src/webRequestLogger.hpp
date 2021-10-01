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
	WebRequestLogger( const std::string& logfilename_, int verbosity, int mask_, int maxnofthreads, int procid, int nofprocs);

	virtual ~WebRequestLogger();

	virtual int level() const noexcept
	{
		return m_level;
	}

	virtual void print( const Level level, const char* tag, const char* msg, size_t msglen) noexcept;

private:
	void logMessage( const char* fmt, ...)
#ifdef __GNUC__
	__attribute__ ((format (printf, 2, 3)))
#endif
	;
 	void logMessageBuf( char* buf, size_t bufsize, const char* fmt, va_list ap);
	const char* getThreadId();
	void alert( const char* msg);
	bool gotAlert() const
	{
		return m_gotAlert;
	}
	void reset();

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
	Level m_level;
	bool m_gotAlert;
	int m_procid;
	int m_nofprocs;
};

}//namespace
#endif


