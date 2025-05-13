output: wadfs

wadfs: wadfs.cpp
	g++ -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 wadfs.cpp -o wadfs -L . -L ../libWad -lfuse -lWad

clean:
	rm -f wadfs

