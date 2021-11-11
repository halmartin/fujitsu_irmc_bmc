#!/bin/sh

ARCH_LC=`echo ${ARCH} | tr '[:upper:]' '[:lower:]'`

./configure --disable-dependency-tracking  --target=$ARCH_LC-linux --build=i686-linux --host=$ARCH_LC-linux CC=$CROSS_COMPILE"gcc" AR=$CROSS_COMPILE"ar" AS=$CROSS_COMPILE"as" LD=$CROSS_COMPILE"ld" NM=$CROSS_COMPILE"nm" RANLIB=$CROSS_COMPILE"ranlib" SIZE=$CROSS_COMPILE"size" STRIP=$CROSS_COMPILE"strip" OBJCOPY=$CROSS_COMPILE"objcopy" CXX=$CROSS_COMPILE"c++" CPP=$CROSS_COMPILE"cpp"

make CC=$CROSS_COMPILE"gcc" AR=$CROSS_COMPILE"ar" AS=$CROSS_COMPILE"as" LD=$CROSS_COMPILE"ld" NM=$CROSS_COMPILE"nm" RANLIB=$CROSS_COMPILE"ranlib" SIZE=$CROSS_COMPILE"size" STRIP=$CROSS_COMPILE"strip" OBJCOPY=$CROSS_COMPILE"objcopy" CXX=$CROSS_COMPILE"c++" CPP=$CROSS_COMPILE"cpp" -j1

make install DESTDIR=${BUILDAREA}/target 

#Copy all the output files to 'OUTPUT' folder
mkdir ${BUILD}/${PACKAGE_NAME}/data/.workspace/libjson/OUTPUT
make install DESTDIR=${BUILD}/${PACKAGE_NAME}/data/.workspace/libjson/OUTPUT
