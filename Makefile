

CC=g++

#all: wxoww

wxoww: wxoww.cpp
	$(CC) -g wxoww.cpp `wx-config --debug --cflags --libs` -o wxoww

clean:
	rm -f *.o wxoww

