#!/bin/bash
mkdir src
cd src
curl -s https://ftp.gnu.org/gnu/binutils/binutils-2.30.tar.xz | tar xvf - -J
curl -s https://ftp.gnu.org/gnu/gcc/gcc-8.1.0/gcc-8.1.0.tar.xz | tar xvf - -J
