/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the context of a delegated request (sub request to another server)
/// \file "webRequestDelegateContext.hpp"
#ifndef _STRUS_WEB_REQUEST_DELEGATE_CONTEXT_IMPL_HPP_INCLUDED
#define _STRUS_WEB_REQUEST_DELEGATE_CONTEXT_IMPL_HPP_INCLUDED
#include "strus/WebRequestDelegateContextInterface.hpp"

namespace strus {

class WebRequestDelegateContext
	:public WebRequestDelegateContextInterface
{
public:
	WebRequestDelegateContext(){}
	~WebRequestDelegateContext(){}
	virtual void putAnswer( const WebRequestAnswer& status);
};

}//namespace
#endif

