/*
---------------------------------------------------------------------
    A web service implementing general search functionality
    using the C++ library strus which implements basic operations
    to build a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey
    Copyright (C) 2015,2016 Andreas Baumann
    Copyright (C) 2015,2016 Eurospider IT AG Zurich

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>

// for now this is the only database implementation in the storage
const std::string DEFAULT_DATABASE						= "leveldb";

// from db_impl.cc, creating a default LRUCache if it is not provided
const int LEVELDB_DATABASE_DEFAULT_LRU_CACHESIZE		= 8 << 20;

// the range is: SanitizeOptions: [ 64 + kNumNonTableCacheFiles, 50000 ]
// kNumNonTableCacheFiles = 10;
// the default is in options.cc: 1000
const int LEVELDB_DATABASE_DEFAULT_MAX_OPEN_FILES		= 1000;

// options.cc: block_size(4096)
const int LEVELDB_DATABASE_DEFAULT_BLOCK_SIZE			= 4096;

// options.cc: write_buffer_size(4<<20)
const int LEVELDB_DATABASE_DEFAULT_WRITE_BUFFER_SIZE	= 4 << 20;

// TODO: this should be 1 for Snappy compression
const int LEVELDB_DATABASE_DEFAULT_COMPRESSION			= true;

#define PACKAGE_STRUS "strus"

const int DEFAULT_PROTOCOL_PRETTY_PRINT					= false;

// TODO: this should be exported IMHO
//#include "strus/versionStorage.hpp"
#define STRUS_VERSION_STRING "0.6.1"

const int DEFAULT_QUERY_FIRST_RANK						= 0;

const int DEFAULT_QUERY_NOF_RANKS						= 10;

const std::string DEFAULT_SUMMARIZER					= "attribute";
const std::string DEFAULT_ATTRIBUTE_DOCID				= "docid";

const std::string DEFAULT_WEIGHTING_SCHEME				= "BM25";
const int DEFAULT_BM25_K1								= 1;
const float DEFAULT_BM25_B								= 0.75;
const int DEFAULT_BM25_AVGDOCLEN						= 1000;
const std::string DEFAULT_BM25_METADATA_DOCLEN				= "doclen";

const int MAX_NOF_FEATURES								= 100;

#endif
