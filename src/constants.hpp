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

// TODO: read from config
#define NOF_THREADS 128

#endif
