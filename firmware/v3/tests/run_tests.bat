REM This script depends on the "Windows Linux Subsystem" being installed
wsl dos2unix -q run_tests.sh
wsl ./run_tests.sh