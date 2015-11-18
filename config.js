{
	"storage" : {
		"basedir" : "./storage",
		"default_create_parameters" : {
			"database" : "leveldb",
			//"cache_size" : 8388608,
			//"max_open_files" : 1000,
			//"write_buffer_size" : 4194304,
			//"block_size" : 4096,
			//"compression" : true,
			"metadata" : [
					{ "name" : "doclen", "type" : "UInt16" },
					{ "name" : "doclen2", "type" : "UInt32" }
			]
		}
	},
	
	"http" : {
		"script" : "/strus"
	},
	
	"service" : {
		"api" : "http",
		"ip" : "0.0.0.0",
		"port" : 8080,
		//"worker_processes" : 5,
		//"worker_threads" : 3
	},
	
	"logging" : {
		"level" : "debug",
		//~ "file" : {
			//~ "name" : "./strusWebService.log",
			//~ "append" : true
		//~ }
	}
}
