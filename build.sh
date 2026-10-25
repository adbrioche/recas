set -eu

cmake -S . -B build
cmake --build build
exec ./build/recas-server "$@"