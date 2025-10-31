mkdir test-runtime
make w16asm-test > /dev/null
cd test-runtime
./w16asm-test
cd ..
rm -rf test-runtime/