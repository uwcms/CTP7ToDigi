#!/bin/bash
mkdir temp;
svn co  svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/rct/RCTCore/src/common temp/ --depth empty;
cd temp;
svn up CTP7Client.cc;
cd ../;
cp temp/* plugins/.;
rm -rf temp;
mkdir temp;
svn co  svn+ssh://svn.cern.ch/reps/cactus/trunk/cactusprojects/rct/RCTCore/include temp/ --depth empty;
cd temp;
svn up CTP7Client.hh;
svn up CTP7.hh;
cd ../;
cp temp/* plugins/.;
rm -rf temp;
