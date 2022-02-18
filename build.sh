#! /bin/bash

cmake -B Build \
    -DUSD_LOCATION=~/Work/WebUsdViewer/Build/Install/USD \
    -DCMAKE_INSTALL_PREFIX=~/Work/WebUsdViewer/Build/Install/USD \
    -DPXR_ENABLE_PYTHON_SUPPORT=1 \
    -DBUILD_MONOLITHIC=ON

cmake --build Build --config Release --target install
