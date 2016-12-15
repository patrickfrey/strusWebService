#!/bin/sh

set -e

OS=$(uname -s)

case $OS in
	Linux)
		# TODO: there is no sign key for cppcms? Using force for now.. :-)
		#curl http://apt.cppcms.com/GPG-key.txt | sudo apt-key add -
		echo "deb http://apt.cppcms.com/ trusty main" | sudo tee -a /etc/apt/sources.list
		sudo apt-get update -qq
		sudo apt-get install -y --force-yes \
			cmake \
			libboost-all-dev \
			libleveldb-dev \
			libcppcms-dev \
			libcurl4-openssl-dev
		;;
		
	Darwin)
		brew update
		if test "X$CC" = "Xgcc"; then
			brew install gcc48 --enable-all-languages || true
			brew link --force gcc48 || true
		fi
		brew install \
			cmake \
			boost \
			gettext \
			snappy \
			leveldb \
			cppcms \
			curl \
			|| true
		# make sure cmake finds the brew version of gettext
		brew link --force gettext || true
		brew link leveldb || true
		brew link snappy || true
		# rebuild leveldb to use gcc-4.8 ABI on OSX (libstc++ differs
		# from stdc++, leveldb uses std::string in API functions, C
		# libraries like gettext and snappy and even boost do not 
		# have this problem)
		if test "X$CC" = "Xgcc"; then
			brew reinstall leveldb --cc=gcc-4.8
		fi
		;;
	
	*)
		echo "ERROR: unknown operating system '$OS'."
		;;
esac

