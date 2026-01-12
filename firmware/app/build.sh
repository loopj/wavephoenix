#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check that BOARD environment variable is set
if [[ -z "$BOARD" ]]; then
    echo "Error: BOARD environment variable not set"
    exit 1
fi

# Generate the makefile project for the specified board
slc generate wavephoenix.slcp \
  --with "$BOARD;wavephoenix" \
  --sdk-extensions=../boards,../libjoybus,../libwavebird \
  -o makefile \
  -d target/$BOARD

# Build the project
make -C target/$BOARD -f wavephoenix.Makefile release

# Generate the .gbl file with Simplicity Commander"
commander gbl create --app target/$BOARD/build/release/wavephoenix.s37 target/$BOARD/build/release/wavephoenix.gbl