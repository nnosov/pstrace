#!/bin/bash

make
gdb --batch --command=script.gdb ./build/trace