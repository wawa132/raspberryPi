#!/bin/bash
# build 폴더가 존재하면 삭제
if [ -d "build" ]; then
    rm -r build
fi

# bin 폴더가 존재하면 내부 파일 삭제
if [ -d "bin" ]; then
    rm -f bin/*
else
    # bin 폴더가 없다면 생성
    mkdir bin
fi

mkdir build
cd build
cmake ..
make
mv ./gateway ../bin
# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/gateway
# sha256sum ../bin/gateway