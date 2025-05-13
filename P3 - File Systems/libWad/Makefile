output: Wad.o libWad.a

wad.o: Wad.cpp Wad.h
	g++ -O -c wad.cpp

libWad.a: Wad.o
	ar cr libWad.a Wad.o
