#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/mnt/Build/Install/USD/lib"
export PYTHONPATH="/mnt/Build/Install/USD/lib/python"

python3 /mnt/Kit/Examples/workspace.py
