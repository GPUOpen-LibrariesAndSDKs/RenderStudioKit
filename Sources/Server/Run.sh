#!/bin/bash

# Get current script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

# Launch server
LD_LIBRARY_PATH=$PARENT_DIR/lib $SCRIPT_DIR/RenderStudioServer
