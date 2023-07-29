CC = g++
TARGET = main
FLAGS = -std=c++2a -O0

headers = groups.h relative.h poly.h test.h
scripts = groups.cpp relative.cpp test.cpp

.main: main.cpp
	$(CC) -o $(TARGET) $< $(headers) $(scripts) $(FLAGS)

all: .main

clean:
	rm main