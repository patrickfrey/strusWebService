Ubuntu 16.04 on x86_64, i686
----------------------------

# Build system
Cmake with gcc or clang. Here in this description we build with 
gcc >= 4.9 (has C++11 support). Unlike other strus projeccts,  strusWebService needs C++11.

# Prerequisites
Install packages with 'apt-get'/aptitude.

## CMake flags
	none besides ordinary flags for stearing the build.

## Required packages
	boost-all >= 1.57
	snappy-dev leveldb-dev libuv-dev cppcms >= 1.2 libcurl4-openssl-dev

### Packages required by cppcms
	zlib1g-dev
	libpcre3-dev

### Download and install cppcms (version 1.2)
	wget https://sourceforge.net/projects/cppcms/files/cppcms/1.2.1/cppcms-1.2.1.tar.bz2
	bzip2 -d cppcms-1.2.1.tar.bz2
	tar -xvf cppcms-1.2.1.tar
	cd cppcms-1.2.1
	cmake .
	make install

# Strus prerequisite packages to install before
	strusBase strus strusAnalyzer strusTrace strusModule strusRpc strusBindings (built with -DWITH_WEBREQUEST=YES)

# Configure build and install strus prerequisite packages with GNU C/C++
	With strusVector and strusPattern enabled:
	for strusprj in strusBase strus strusAnalyzer strusTrace \
		strusModule strusRpc strusBindings
	do
	git clone https://github.com/patrickfrey/$strusprj $strusprj
	cd $strusprj
	cmake -DCMAKE_BUILD_TYPE=Release -DLIB_INSTALL_DIR=lib .
	make
	make install
	cd ..
	done

# Fetch sources
	git clone https://github.com/patrickfrey/strusWebService
	cd strusWebService
	git submodule update --init --recursive
	git submodule foreach --recursive git checkout master
	git submodule foreach --recursive git pull

# Configure build and install strus prerequisite packages with Clang C/C++
	Minimal build with a reasonable default for library installation directory:
	for strusprj in strusBase strus strusAnalyzer strusTrace \
		strusModule strusRpc
	do
	git clone https://github.com/patrickfrey/$strusprj $strusprj
	cd $strusprj
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" .
	make
	make install
	cd ..
	done

# Configure with GNU C/C++
	Minimal build with setting the installation directory:
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DLIB_INSTALL_DIR=lib .

# Configure with Clang C/C++
	Minimal build, only Lua bindings without Vector and Pattern and
	a reasonable default for library installation directory:

	cmake -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" .

# Build
	make

# Run tests
	make test

# Install
	make install

# Use the perl client
	The perl client in ./client_perl/ needs the REST::Client perl module

	apt install cpanminus
	cpanm JSON
	cpanm REST::Client
	cpanm URI::Encode



