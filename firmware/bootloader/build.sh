#!/usr/bin/env bash
set -e

# Check that BOARD environment variable is set
if [[ -z "$BOARD" ]]; then
    echo "Error: BOARD environment variable not set"
    exit 1
fi

# Generate the makefile project for the specified board
slc generate bootloader.slcp \
  --with "$BOARD;wavephoenix" \
  --sdk-extensions=../boards \
  -o makefile \
  -d target/$BOARD

# Build the project
make -C target/$BOARD -f bootloader.Makefile release