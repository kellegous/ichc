
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

if [ ! -d zlib ]; then
  svn co http://src.chromium.org/svn/trunk/src/third_party/zlib
fi

cd ..
mkdir -p chrome/common
if [ ! -f chrome/common/zip.cc ]; then
  svn cat http://src.chromium.org/svn/trunk/src/chrome/common/zip.cc > chrome/common/zip.cc
fi

if [ ! -f chrome/common/zip.h ]; then
  svn cat http://src.chromium.org/svn/trunk/src/chrome/common/zip.h > chrome/common/zip.h
fi

mkdir -p net/base
if [ ! -f net/base/net_errors.h ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/net_errors.h > net/base/net_errors.h
fi
if [ ! -f net/base/net_error_list.h ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/net_error_list.h > net/base/net_error_list.h
fi
if [ ! -f net/base/completion_callback.h ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/completion_callback.h > net/base/completion_callback.h
fi
if [ ! -f net/base/file_stream.h ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/file_stream.h > net/base/file_stream.h
fi
if [ ! -f net/base/file_stream_posix.cc ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/file_stream_posix.cc > net/base/file_stream_posix.cc
fi
if [ ! -f net/base/file_stream_win.cc ]; then
  svn cat http://src.chromium.org/svn/trunk/src/net/base/file_stream_win.cc > net/base/file_stream_win.cc
fi

./build/gyp_chromium ichc.gyp
