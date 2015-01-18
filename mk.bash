insmod ./hw4.o iKey=3
cp os4-tests-master/ / -r
cd /os4-tests-master
make clean
make check
