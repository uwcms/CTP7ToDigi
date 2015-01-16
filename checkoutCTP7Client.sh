#!/bin/bash

pushd ../
svn co  svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/rct/CTP7Play;
cd CTP7Play
./buildCTP7Client 
popd

mkdir temp;
svn co  svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/rct/RCTCore/src/common temp/ --depth empty;
cd temp;
svn up CTP7Client.cc;
svn up RCTInfoFactory.cc;
cd ../;
cp temp/* plugins/.;
rm -rf temp;

mkdir temp;
svn co  svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/rct/RCTCore/include temp/ --depth empty;
cd temp;
svn up CTP7Client.hh;
svn up CTP7.hh;
svn up RCTInfoFactory.hh;
svn up RCTInfo.hh;
cd ../;
cp temp/* plugins/.;
rm -rf temp;

