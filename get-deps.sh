
mkdir -p src/tools
mkdir -p src/third_party

cd src

# BUILD
if [ ! -d build ]; then
  svn co http://src.chromium.org/svn/trunk/src/build
fi

# BASE
if [ ! -d base ]; then
  svn co http://src.chromium.org/svn/trunk/src/base
fi

# TESTING
if [ ! -d testing ]; then
  svn co http://src.chromium.org/svn/trunk/src/testing
fi

cd testing
if [ ! -d gtest ]; then
  svn co http://googletest.googlecode.com/svn/trunk gtest
fi

# TOOLS
cd ../tools
if [ ! -d gyp ]; then
  svn co http://gyp.googlecode.com/svn/trunk gyp
fi

# THIRD_PARTY
cd ../third_party
if [ ! -d icu ]; then
  svn co http://src.chromium.org/svn/trunk/deps/third_party/icu42 icu
fi

if [ ! -d modp_b64 ]; then
  svn co http://src.chromium.org/svn/trunk/src/third_party/modp_b64
fi

if [ ! -d libevent ]; then
  svn co http://src.chromium.org/svn/trunk/src/third_party/libevent
fi

cd ..
./build/gyp_chromium base/base.gyp
