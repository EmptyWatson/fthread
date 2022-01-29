#! /bin/sh

source  /opt/rh/devtoolset-9/enable 

mkdir ext
cd ext
git clone git://github.com/AmokHuginnsson/replxx.git
cd ..

mkdir -p build
cd build

cmake ..
make -j
