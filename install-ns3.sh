#! /bin/bash

#script to auto-install ns-3.  assumes that boost is built by source and installed to the home directory under ~/boost/include and ~/lib

cd /scratch/kebenson
wget 'http://www.nsnam.org/releases/latest'
tar -xvf ns-allinone-3.*.tar.bz2
cd ns-allinone-3.*
./build.py #this will fail, but we need to get the other parts built

#some funny build configurations were required when I built boost from source
cd ns-3.*
#LDFLAGS_EXTRA="-L$HOME/lib" CXXFLAGS_EXTRA="-I$HOME/boost/include" python waf configure --enable-examples --enable-tests --boost-includes=/home/kebenson/boost/include/ --boost-libs=/home/kebenson/lib/
./waf configure --enable-examples --enable-tests
./waf build
./test.py
ln -s ~/ron_output ron_output
