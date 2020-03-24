#!/bin/bash

bench() {
    eval "$@"
    make -j 8
    echo "bench config $@" >> bench_result.txt
    ./bench_internal | egrep "field_sqr:|field_mul|scalar_mul|scalar_sqr|group_double|group_add" >> bench_result.txt
    ./bench_sign >> bench_result.txt
    ./bench_verify >> bench_result.txt
    echo >> bench_result.txt
    make clean
}
make clean
export SECP256K1_BENCH_ITERS=50000
echo "SECP256K1_BENCH_ITERS=SECP256K1_BENCH_ITERS" > bench_result.txt
echo >> bench_result.txt
bench CFLAGS="-DUSE_ASM_X86_64_FIELD" ./configure --disable-openssl-tests --with-asm=x86_64
bench CFLAGS="" ./configure --disable-openssl-tests --without-asm
bench CFLAGS="" ./configure --disable-openssl-tests --with-asm=x86_64

bench CC="clang" CFLAGS="-DUSE_ASM_X86_64_FIELD" ./configure --disable-openssl-tests --with-asm=x86_64
bench CC="clang" CFLAGS="" ./configure --disable-openssl-tests --without-asm
bench CC="clang" CFLAGS="" ./configure --disable-openssl-tests --with-asm=x86_64

bench CFLAGS="-DUSE_ASM_X86_64_FIELD" ./configure --disable-openssl-tests --with-asm=x86_64 --enable-endomorphism
bench CFLAGS="" ./configure --disable-openssl-tests --without-asm --enable-endomorphism
bench CFLAGS="" ./configure --disable-openssl-tests --with-asm=x86_64 --enable-endomorphism
