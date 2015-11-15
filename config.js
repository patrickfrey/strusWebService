{
	"storage" : {
		"basedir" : "./storage",
		"default_create_parameters" : {
			//"cache_size" : 8388608,
			//"max_open_files" : 1000,
			//"write_buffer_size" : 4194304,
			//"block_size" : 4096,
			//"compression" : true,
			"metadata" : [
					{ "name" : "doclen", "type" : "UINT16" },
					{ "name" : "doclen2", "type" : "UINT32" }
			]
		}
	},
	
	"service" : {
		"api" : "http",
		"ip" : "0.0.0.0",
		"port" : 8080
	}
}
