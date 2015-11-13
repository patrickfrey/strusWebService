#ifndef ERROR_CODES_H
#define ERROR_CODES_H

const int ERROR_INDEX 									= 1000;
const int ERROR_INDEX_CREATE_CMD						= ERROR_INDEX + 100;
const int ERROR_INDEX_CREATE_DATABASE_EXISTS			= ERROR_INDEX_CREATE_CMD + 1;
const int ERROR_INDEX_CREATE_DATABASE_INTERFACE 		= ERROR_INDEX_CREATE_CMD + 2;
const int ERROR_INDEX_CREATE_STORAGE_INTERFACE	 		= ERROR_INDEX_CREATE_CMD + 3;
const int ERROR_INDEX_CREATE_CMD_CREATE_DATABASE		= ERROR_INDEX_CREATE_CMD + 4;
const int ERROR_INDEX_CREATE_CMD_CREATE_CLIENT			= ERROR_INDEX_CREATE_CMD + 5;
const int ERROR_INDEX_CREATE_CMD_MKDIR_STORAGE_DIR		= ERROR_INDEX_CREATE_CMD + 6;
const int ERROR_INDEX_DESTROY_CMD 						= ERROR_INDEX + 200;
const int ERROR_INDEX_DESTROY_CMD_NO_SUCH_DATABASE		= ERROR_INDEX_DESTROY_CMD + 1;
const int ERROR_INDEX_DESTROY_CMD_DESTROY_DATABASE		= ERROR_INDEX_DESTROY_CMD + 2;

#endif
