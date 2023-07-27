CC = g++
TARGET = main
FLAGS = -std=c++2a -O0

headers = groupGenerators.h relative.h poly.h test.h
scripts = groupGenerators.cpp groups.cpp relative.cpp test.cpp

.main: main.cpp
	$(CC) -o $(TARGET) $< $(headers) $(scripts) $(FLAGS)

clean:
	rm main