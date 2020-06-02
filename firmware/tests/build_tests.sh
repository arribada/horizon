hash dos2unix 2>/dev/null

if [ "$?" = "0" ]
then
    dos2unix -q build_tests.sh
    dos2unix -q run_tests.sh
    dos2unix -q src/build_gtest.sh
    dos2unix -q src/generate_mocks.sh
else
    echo "dos2unix not installed, skipping line ending checks"
fi

cd src
bash ./build_gtest.sh
bash ./generate_mocks.sh
mkdir -p build
cd build
cmake ..  # Must use cmake version 3.10 or later
make -j$(nproc)
MAKE_RET=$?
cd ../..
exit "$MAKE_RET"