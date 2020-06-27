# Usage:
# Call with no arguments for a regular test
# Call with --intensive to repeat each test 4 times in random order

cd src/build
if [ -z "$1" ]; then
    echo "Running standard test"
    ctest --no-compress-output --schedule-random --timeout 30 -T Test -j$(nproc) || true # Always return true to stop jenkins halting here
elif [ $1 == "--intensive" ]; then
    echo "Running intensive test"
    ctest --no-compress-output --repeat-until-fail 4 --schedule-random --timeout 60 -T Test -j$(nproc) || true # Always return true to stop jenkins halting here
else
    echo "Command \"$1\" not recognised"
fi
cd ..