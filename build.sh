#!/bin/bash

MODE=$1
TARGET=$2
EXTRA_ARGS=${@:3}

if [[ $MODE == __UNDEFINED__ ]] || [[ $MODE == "" ]] || ( [[ $MODE != "Debug" ]] && [[ $MODE != "Release" ]])
then
    echo "Please provide as first argument Debug or Release"
    exit 1
fi

mkdir -p build/$MODE
cd build/$MODE

if command -v ninja &> /dev/null
then
    BUILD_ENGINE="-GNinja"
else
    BUILD_ENGINE=""
fi

case "$OSTYPE" in
  darwin*)  cmake $BUILD_ENGINE $COMMON_FLAGS $SOLVER_FLAGS $EXTRA_ARGS ../.. ;;
  linux*)  cmake $BUILD_ENGINE $COMMON_FLAGS $SOLVER_FLAGS $LICENSE_FLAGS $EXTRA_ARGS ../.. ;;
  msys*)    cmake.exe -G "Visual Studio 16 2019 Win64" $COMMON_FLAGS $SOLVER_FLAGS $EXTRA_ARGS ..\\.. ;;
  *)        echo "unknown: $OSTYPE" ;;
esac


numcores=$([ $(uname) = 'Darwin' ] &&
                       sysctl -n hw.physicalcpu_max ||
                       lscpu -p | grep -v '^#' | cut -d, -f2 | sort -u | wc -l)


if [[ $TARGET == __UNDEFINED__ ]] || [[ $TARGET == "" ]] || [[ $TARGET == "ALL" ]]
then
    cmake --build . --config $MODE -j $numcores
else
    cmake --build . --config $MODE --target $TARGET -j $numcores
fi

cd ../..

