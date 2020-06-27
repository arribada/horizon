REM This script depends on the "Windows Linux Subsystem" being installed
wsl dos2unix -q build_tests.sh
wsl dos2unix -q src/build_gtest.sh
wsl dos2unix -q src/generate_mocks.sh
wsl ./build_tests.sh