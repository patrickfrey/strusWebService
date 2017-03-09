{	
	"storage" : {
		"basedir" : "./storage",
		"default_create_parameters" : {
			"database" : "leveldb",
			"cache_size" : 8388608,
			"max_open_files" : 512,
			"write_buffer_size" : 8192,
			"block_size" : 4096,
			"compression" : true,
			"metadata" : [
					{ "name" : "doclen", "type" : "UInt16" },
					{ "name" : "docweight", "type" : "Float32" },
					{ "name" : "date", "type" : "UInt16" }
			]
		}
	},
	
	"transactions" : {
		"max_idle_time" : 120
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
		// setting anything but 0 or 1 is illegal currently!
		"worker_processes" : 0,
		"worker_threads" : 4,
		"applications_pool_size" : 4
	},
	
	"security" : {
		"content_length_limit": 16384000,
		"cors" : {
			"enable" : true,
			"allowed_origins" : [
				"http://eeepc.lan:1313",
				"http://localhost:1313"
			],
			"age" : 10
		}
	},
	
	"logging" : {
		"level" : "info",
		//~ "file" : {
			//~ "name" : "./strusWebService.log",
			//~ "append" : true
		//~ }
	},
	
	"democlient" : {
		"enable" : true,
		"basedir" : "./democlient"
	},
	
	"debug" : {
		//~ "log_requests" : true,
		//~ "request_file" : "./requests.log",
		"protocol" : {
			//~ "pretty_print" : true,
			//~ "enable_quit_command" : true
		}
	}		
}
