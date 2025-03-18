#!/bin/bash
rm -r build
rm -f bin/*
mkdir build
cd build
cmake ..
make
mv ./gateway ../bin
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/gateway
sha256sum ../bin/gateway