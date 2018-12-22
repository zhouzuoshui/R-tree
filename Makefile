objects = draw.o
libraries = libqrtree.so libqrnode.so
CC = g++
FLAGS = -std=c++14 -g
LIBFLAGS = -g -std=c++14 -fPIC -shared 


main: libqrnode.so libqrtree.so draw.o
	$(CC) $(FLAGS) -L. -lqrtree -lqrnode draw.o -o draw `pkg-config --cflags --libs opencv`

draw.o: qrnode.hpp qrtree.hpp draw.cpp 
	$(CC) $(FLAGS)  -c draw.cpp -o draw.o

libqrtree.so: qrnode.hpp qrtree.hpp qrtree.cpp qrnode.cpp
	$(CC) $(LIBFLAGS) qrtree.cpp -o libqrtree.so

libqrnode.so: qrnode.cpp qrnode.hpp
	$(CC) $(LIBFLAGS) qrnode.cpp -o libqrnode.so
	
.PHONY: clean	
clean:
	rm main $(objects) $(libraries) draw
