#! /bin/bash

#script to auto-install ns-3.  *NO LONGER* assumes that boost is built by source and installed to the home directory under ~/boost/include and ~/lib

#cd /scratch/kebenson
#wget 'http://www.nsnam.org/releases/latest'
wget 'https://www.nsnam.org/release/ns-allinone-3.16.tar.bz2' #latest doesn't have an actual non-version-specific link name
tar -xvf ns-allinone-3.*.tar.bz2
cd ns-allinone-3.*
cd ns-3.*

#need to clone the repo
#but into an empty directory
mkdir .tempns3
cd .tempns3

git clone git@github.com:KyleBenson/ns3.git
cd ns3
git checkout geocron
git reset --hard HEAD
cd ..

#move all the git updated ns3 files to the fresh release ns3 folder
mv ns3/* ../..
mv ns3/.git* ../.. #above doesn't get the hiddens
rm -rf ns3
cd .. #out of temp file
rm -rf .tempns3

./build.py #this will fail, but we need to get the other parts built

#some funny build configurations were required when I built boost from source
cd ns-3.*
#LDFLAGS_EXTRA="-L$HOME/lib" CXXFLAGS_EXTRA="-I$HOME/boost/include" python waf configure --enable-examples --enable-tests --boost-includes=/home/kebenson/boost/include/ --boost-libs=/home/kebenson/lib/
./waf configure --enable-examples --enable-tests
./waf build
./test.py
ln -s ~/ron_output ron_output
