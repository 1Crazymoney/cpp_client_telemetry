#!/bin/bash

export PATH=/usr/local/bin:$PATH

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current directory: $DIR"
cd $DIR

if [ "$1" == "clean" ]; then
 rm -f CMakeCache.txt *.cmake
 rm -rf out
 rm -rf .buildtools
# make clean
fi

# Install build tools and recent sqlite3
FILE=.buildtools
OS_NAME=`uname -a`
if [ ! -f $FILE ]; then
case "$OS_NAME" in
 *Darwin*) tools/setup-buildtools-mac.sh ;;
 *Linux*)  sudo tools/setup-buildtools.sh ;;
 *)        echo "WARNING: unsupported OS $OS_NAME , skipping build tools installation.."
esac
# Assume that the build tools have been successfully installed
echo > $FILE
fi

if [ -f /usr/bin/gcc ]; then
echo "gcc   version: `gcc --version`"
fi

if [ -f /usr/bin/clang ]; then
echo "clang version: `clang --version`"
fi

#rm -rf out
mkdir -p out
cd out

# .tgz package
CMAKE_PACKAGE_TYPE=tgz

# .deb package
if [ -f /usr/bin/dpkg ]; then
export CMAKE_PACKAGE_TYPE=deb
fi

# .rpm package
if [ -f /usr/bin/rpmbuild ]; then
export CMAKE_PACKAGE_TYPE=rpm
fi

if [ "$2" == "release" ]; then
# TODO: pass custom build flags?
  cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_PACKAGE_TYPE=$CMAKE_PACKAGE_TYPE ..
# TODO: strip symbols to minimize
else
  cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PACKAGE_TYPE=$CMAKE_PACKAGE_TYPE ..
fi

# Build all
# TODO: what are the pros and cons of using 'make' vs 'cmake --build' ?
#make
cmake --build .

# Remove old package
rm -f *.deb *.rpm

# Build new package
make package

# Debian / Ubuntu / Raspbian
if [ -f /usr/bin/dpkg ]; then
# Install new package
sudo dpkg -i *.deb
fi

# RedHat / CentOS
if [ -f /usr/bin/rpmbuild ]; then
sudo rpm -i --force -v *.rpm
fi

# Install SDK headers and lib to /usr/local
#
## TODO: [MG] - fix this section for shared library
## strip --strip-unneeded out/lib/libaria.so
## strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag out/lib/libaria.so

if [ "$CMAKE_PACKAGE_TYPE" == "tgz" ]; then
cd ..
ARIA_SDK_INSTALL_DIR="${ARIA_SDK_INSTALL_DIR:-/usr/local}"
sudo ./install.sh $ARIA_SDK_INSTALL_DIR
fi
