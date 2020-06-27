wget -q -N -O gtest.tar.gz https://github.com/google/googletest/archive/release-1.8.1.tar.gz
tar -zxf gtest.tar.gz
rm gtest.tar.gz
cd googletest*
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ../..