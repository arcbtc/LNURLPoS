#!/bin/bash

clang++ run-tests.cpp QrCode.cpp QrSegment.cpp BitBuffer.cpp ../src/qrcoded.c -o test && ./test
clang++ run-tests.cpp QrCode.cpp QrSegment.cpp BitBuffer.cpp ../src/qrcoded.c -o test -D LOCK_VERSION=3 && ./test

