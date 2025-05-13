output: MemoryManager.o FitFunctions.o libMemoryManager.a

MemoryManager.o: MemoryManager.cpp MemoryManager.h
	g++ -O -c MemoryManager.cpp

FitFunctions.o: FitFunctions.cpp FitFunctions.h
	g++ -O -c FitFunctions.cpp

libMemoryManager.a: MemoryManager.o FitFunctions.o
	ar cr libMemoryManager.a MemoryManager.o FitFunctions.o

clean:
	rm -f *.o libMemoryManager.a
