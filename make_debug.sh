#!/usr/bin/env bash

CMAKE_BUILD_TYPE=Debug
BUILD_DIR=build.`echo ${CMAKE_BUILD_TYPE} | tr '[:upper:]' '[:lower:]'`
rm -fr ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ../
#make -j 4 package
make -j 4
popd
