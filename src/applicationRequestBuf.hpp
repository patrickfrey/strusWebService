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
/// \brief Abstraction for a request of a cppcms application for strus with its data
#ifndef _STRUS_WEBSERVICE_APPLICATION_REQUEST_BUF_HPP_INCLUDED
#define _STRUS_WEBSERVICE_APPLICATION_REQUEST_BUF_HPP_INCLUDED
#include "strus/webRequestContextInterface.hpp"
#include "strus/webRequestContent.hpp"
#include "strus/base/unique_ptr.hpp"
#include <cppcms/http_content_type.h>
#include <string>
#include <utility>

///\brief Forward declaration
class ServiceClosure;

class ApplicationRequestBuf
{
public:
	ApplicationRequestBuf( const ServiceClosure* service,
			const std::string& contextname,
			const std::string& schemaname,
			const std::string& argument,
			const std::string& accepted_charset_,
			const std::string& accepted_doctype_,
			const std::string& role_,
			const cppcms::http::content_type& content_type,
			const std::pair<void*,size_t>& content_data);

	const char* http_accept() const			{return m_accepted_doctype.c_str();}
	const char* http_accept_charset() const		{return m_accepted_charset.c_str();}

	bool ok() const					{return m_answer.ok();}
	const strus::WebRequestAnswer& answer() const	{return m_answer;}
	const strus::WebRequestContent& content() const	{return m_content;}

	void execute();
	void debug();

private:
	strus::unique_ptr<strus::WebRequestContextInterface> m_context;
	strus::WebRequestContent m_content;
	strus::WebRequestAnswer m_answer;
	std::string m_accepted_charset;
	std::string m_accepted_doctype;
	std::string m_charset;
	std::string m_doctype;
	std::string m_role;
};

#endif

