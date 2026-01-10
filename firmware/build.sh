#!/usr/bin/env bash

# Check that BOARD environment variable is set
if [[ -z "$BOARD" ]]; then
    echo "Error: BOARD environment variable not set"
    exit 1
fi

# Clean previous build artifacts
rm -rf target/$BOARD

# Generate the CMake project for the specified board
slc generate wavephoenix.slcp --with "$BOARD;wavephoenix" --export-destination target/$BOARD --output-type cmake --sdk-extensions=.,libsi,libwavebird

# Build the project
cd target/$BOARD/wavephoenix_cmake
cmake --workflow --preset project

# Generate the .gbl file
/Applications/Commander.app/Contents/MacOS/commander gbl create \
  --app build/default_config/wavephoenix.s37 \
  build/default_config/wavephoenix.gbl