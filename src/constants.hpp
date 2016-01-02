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
#define STRUS_VERSION_STRING "0.4.0"

const int DEFAULT_QUERY_FIRST_RANK						= 0;

const int DEFAULT_QUERY_NOF_RANKS						= 10;

const std::string DEFAULT_SUMMARIZER					= "attribute";
const std::string DEFAULT_ATTRIBUTE_DOCID				= "docid";

const std::string DEFAULT_WEIGHTING_SCHEME				= "BM25";
const int DEFAULT_BM25_K1								= 1;
const float DEFAULT_BM25_B								= 0.75;
const int DEFAULT_BM25_AVGDOCLEN						= 1000;
const std::string DEFAULT_BM25_METADATA_DOCLEN				= "doclen";

#endif
