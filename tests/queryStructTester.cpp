/*
 * Copyright (c) 2014 Patrick P. Frey
 * Copyright (c) 2015,2016 Andreas Baumann
 * Copyright (c) 2015,2016 Eurospider IT AG Zurich
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "query.hpp"

int main( void )
{
	std::vector<struct Feature *> features;
	
	TermFeature *feature = new TermFeature( "feat", Term( "word", "hello" ), 1.0 );
	features.push_back( feature );

	std::vector<boost::shared_ptr<Node> > terms;
	terms.push_back( boost::shared_ptr<Node>( new Term( "word", "hello" ) ) );
	ExpressionFeature *expr = new ExpressionFeature( "sel", Expression( "contains", 0, 0, terms ), 1.0 );
	features.push_back( expr );
	delete expr;
	
	QueryRequest qry; 
	QueryRequest default_qry_req( qry );
	QueryRequest qry_req = default_qry_req;		
	
	return 0;
}
