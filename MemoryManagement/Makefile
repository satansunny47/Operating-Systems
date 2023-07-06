mergesort: mergesort.cpp libgoodmalloc.a
	g++ -w -o mergesort mergesort.cpp libgoodmalloc.a -pthread
	
libgoodmalloc.a: goodmalloc.o
	ar -rcs libgoodmalloc.a goodmalloc.o

goodmalloc.o: goodmalloc.h goodmalloc.cpp
	g++ -w -c goodmalloc.cpp

clean:
	rm -rf *.o *.a mergesort