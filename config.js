{
	"protocol" : {
		//"pretty_print" : false
	},
	
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
					{ "name" : "docweight", "type" : "Float32" },
					{ "name" : "date", "type" : "UInt16" }
			]
		}
	},
	
	"extensions" : {
		"directory" : "/usr/lib64/strus/modules",
		"modules" : [
			"modtest.so"
		]
	},
	
	"http" : {
		"script" : "/strus"
	},
	
	"service" : {
		"api" : "http",
		"ip" : "0.0.0.0",
		"port" : 8080,
		//~ worker_processes : 5,
		//~ worker_threads : 25
	},
	
	"security" : {
		"content_length_limit": 16384000
	},
	
	"logging" : {
		"level" : "debug",
		//~ "file" : {
			//~ "name" : "./strusWebService.log",
			//~ "append" : true
		//~ }
	}
}
