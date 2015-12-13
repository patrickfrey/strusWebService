#ifndef ERROR_CODES_H
#define ERROR_CODES_H

const int ERROR_BASE										= 1000;

// generic protocol errors

const int ERROR_ILLEGAL_METHOD								= ERROR_BASE + 1;
const int ERROR_IILLEGAL_JSON								= ERROR_BASE + 2;
const int ERROR_NOT_IMPLEMENTED								= ERROR_BASE + 3;

// errors in index management

const int ERROR_INDEX 										= 2000;

const int ERROR_INDEX_ILLEGAL_JSON							= ERROR_INDEX + 1;

const int ERROR_INDEX_CREATE_CMD							= ERROR_INDEX + 100;
const int ERROR_INDEX_CREATE_DATABASE_EXISTS				= ERROR_INDEX_CREATE_CMD + 1;
const int ERROR_INDEX_CREATE_DATABASE_INTERFACE 			= ERROR_INDEX_CREATE_CMD + 2;
const int ERROR_INDEX_CREATE_STORAGE_INTERFACE	 			= ERROR_INDEX_CREATE_CMD + 3;
const int ERROR_INDEX_CREATE_CMD_CREATE_DATABASE			= ERROR_INDEX_CREATE_CMD + 4;
const int ERROR_INDEX_CREATE_CMD_CREATE_CLIENT				= ERROR_INDEX_CREATE_CMD + 5;
const int ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR			= ERROR_INDEX_CREATE_CMD + 6;

const int ERROR_INDEX_DESTROY_CMD 							= ERROR_INDEX + 200;
const int ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE			= ERROR_INDEX_DESTROY_CMD + 1;
const int ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE			= ERROR_INDEX_DESTROY_CMD + 2;

const int ERROR_INDEX_STATS_CMD								= ERROR_INDEX + 300;
const int ERROR_INDEX_STATS_CMD_ILLEGAL_STORAGE_DIR			= ERROR_INDEX_STATS_CMD + 1;
const int ERROR_INDEX_STATS_CMD_NO_SUCH_DATABASE			= ERROR_INDEX_STATS_CMD + 2;
const int ERROR_INDEX_STATS_CMD_CREATE_DATABASE_CLIENT		= ERROR_INDEX_STATS_CMD + 3;
const int ERROR_INDEX_STATS_CMD_CREATE_STORAGE_CLIENT		= ERROR_INDEX_STATS_CMD + 4;

const int ERROR_INDEX_CONFIG_CMD							= ERROR_INDEX + 400;
const int ERROR_INDEX_CONFIG_CMD_ILLEGAL_STORAGE_DIR		= ERROR_INDEX_CONFIG_CMD + 1;
const int ERROR_INDEX_CONFIG_CMD_NO_SUCH_DATABASE			= ERROR_INDEX_CONFIG_CMD + 2;
const int ERROR_INDEX_CONFIG_CMD_CREATE_DATABASE_CLIENT		= ERROR_INDEX_CONFIG_CMD + 3;
const int ERROR_INDEX_CONFIG_CMD_CREATE_STORAGE_CLIENT		= ERROR_INDEX_CONFIG_CMD + 4;
const int ERROR_INDEX_CONFIG_CMD_CREATE_METADATA_READER		= ERROR_INDEX_CONFIG_CMD + 5;

// errors in document management

const int ERROR_DOCUMENT									= 3000;

const int ERROR_DOCUMENT_INSERT_CMD							= ERROR_DOCUMENT + 100;
const int ERROR_DOCUMENT_INSERT_CMD_NO_SUCH_DATABASE		= ERROR_DOCUMENT_INSERT_CMD + 1;
const int ERROR_DOCUMENT_INSERT_CMD_CREATE_DATABASE_CLIENT	= ERROR_DOCUMENT_INSERT_CMD + 2;
const int ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_CLIENT	= ERROR_DOCUMENT_INSERT_CMD + 3;
const int ERROR_DOCUMENT_INSERT_CMD_CREATE_STORAGE_TRANSACTION = ERROR_DOCUMENT_INSERT_CMD + 4;
const int ERROR_DOCUMENT_INSERT_ILLEGAL_JSON				= ERROR_DOCUMENT_INSERT_CMD + 5;	
const int ERROR_DOCUMENT_INSERT_CMD_DOCID_REQUIRED			= ERROR_DOCUMENT_INSERT_CMD + 6;
const int ERROR_DOCUMENT_INSERT_TOO_BIG_POSITION			= ERROR_DOCUMENT_INSERT_CMD + 7;

const int ERROR_DOCUMENT_DELETE_CMD 						= ERROR_DOCUMENT + 200;
const int ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DATABASE		= ERROR_DOCUMENT_DELETE_CMD + 1;
const int ERROR_DOCUMENT_DELETE_CMD_CREATE_DATABASE_CLIENT	= ERROR_DOCUMENT_DELETE_CMD + 2;
const int ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_CLIENT	= ERROR_DOCUMENT_DELETE_CMD + 3;
const int ERROR_DOCUMENT_DELETE_CMD_NO_SUCH_DOCUMENT		= ERROR_DOCUMENT_DELETE_CMD + 4;
const int ERROR_DOCUMENT_DELETE_CMD_CREATE_STORAGE_TRANSACTION = ERROR_DOCUMENT_DELETE_CMD + 5;
const int ERROR_DOCUMENT_DELETE_ILLEGAL_JSON				= ERROR_DOCUMENT_DELETE_CMD + 6;	
const int ERROR_DOCUMENT_DELETE_CMD_DOCID_REQUIRED			= ERROR_DOCUMENT_DELETE_CMD + 7;

const int ERROR_DOCUMENT_GET_CMD							= ERROR_DOCUMENT + 300;
const int ERROR_DOCUMENT_GET_CMD_NO_SUCH_DATABASE			= ERROR_DOCUMENT_GET_CMD + 1;
const int ERROR_DOCUMENT_GET_CMD_CREATE_DATABASE_CLIENT		= ERROR_DOCUMENT_GET_CMD + 2;
const int ERROR_DOCUMENT_GET_CMD_CREATE_STORAGE_CLIENT		= ERROR_DOCUMENT_GET_CMD + 3;
const int ERROR_DOCUMENT_GET_CMD_CREATE_STORAGE_TRANSACTION = ERROR_DOCUMENT_GET_CMD + 4;
const int ERROR_DOCUMENT_GET_ILLEGAL_JSON					= ERROR_DOCUMENT_GET_CMD + 5;	
const int ERROR_DOCUMENT_GET_CMD_CREATE_METADATA_READER		= ERROR_DOCUMENT_GET_CMD + 6;
const int ERROR_DOCUMENT_GET_CMD_NO_SUCH_DOCUMENT			= ERROR_DOCUMENT_GET_CMD + 7;
const int ERROR_DOCUMENT_GET_CMD_CREATE_ATTRIBUTE_READER	= ERROR_DOCUMENT_GET_CMD + 8;

const int ERROR_DOCUMENT_EXISTS_CMD							= ERROR_DOCUMENT + 400;
const int ERROR_DOCUMENT_EXISTS_CMD_NO_SUCH_DATABASE		= ERROR_DOCUMENT_EXISTS_CMD + 1;
const int ERROR_DOCUMENT_EXISTS_CMD_CREATE_DATABASE_CLIENT	= ERROR_DOCUMENT_EXISTS_CMD + 2;
const int ERROR_DOCUMENT_EXISTS_CMD_CREATE_STORAGE_CLIENT	= ERROR_DOCUMENT_EXISTS_CMD + 3;
const int ERROR_DOCUMENT_EXISTS_CMD_CREATE_STORAGE_TRANSACTION = ERROR_DOCUMENT_EXISTS_CMD + 4;
const int ERROR_DOCUMENT_EXISTS_ILLEGAL_JSON				= ERROR_DOCUMENT_EXISTS_CMD + 5;	

// errors in query

const int ERROR_QUERY_CMD									= 4000;

const int ERROR_QUERY_CMD_NO_SUCH_DATABASE					= ERROR_QUERY_CMD + 1;
const int ERROR_QUERY_CMD_CREATE_DATABASE_CLIENT			= ERROR_QUERY_CMD + 2;
const int ERROR_QUERY_CMD_CREATE_STORAGE_CLIENT				= ERROR_QUERY_CMD + 3;
const int ERROR_QUERY_CMD_GET_ILLEGAL_JSON					= ERROR_QUERY_CMD + 4;
const int ERROR_QUERY_CMD_CREATE_QUERY_EVAL_INTERFACE		= ERROR_QUERY_CMD + 5;
const int ERROR_QUERY_CMD_CREATE_QUERY						= ERROR_QUERY_CMD + 6;
const int ERROR_QUERY_CMD_CREATE_QUERY_PROCESSOR			= ERROR_QUERY_CMD + 7;
const int ERROR_QUERY_CMD_GET_WEIGHTING_FUNCTION			= ERROR_QUERY_CMD + 8;
const int ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION			= ERROR_QUERY_CMD + 9;
const int ERROR_QUERY_CMD_GET_SUMMARIZER_FUNCTION_INSTANCE	= ERROR_QUERY_CMD + 10;
const int ERROR_QUERY_CMD_QUERY_EVALUATE					= ERROR_QUERY_CMD + 11;

// errors in other commands

const int ERROR_OTHER_CMD									= 5000;

const int ERROR_OTHER_CMD_CREATE_QUERY_PROCESSOR			= ERROR_OTHER_CMD + 1;

#endif
