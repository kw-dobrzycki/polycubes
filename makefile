CC = g++
TARGET = polycube
TEST = test
LIB = polycube
CFLAGS = -I$(HDIR) -std=c++2a -O3 -fopenmp

# directories for library, this program is only main.
SRCDIR = src
HDIR = include
BUILDDIR = build

SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRC))
HEADERS := $(wildcard $(HDIR)/*.h) $(wildcard $(SRCDIR)/*.h)

.PHONY: all test library clean

all: main.cpp library
	$(CC) -o $(TARGET) $< -L$(BUILDDIR) -l$(LIB) $(CFLAGS)

library: $(OBJ) build
	ar rcs -o $(BUILDDIR)/lib$(LIB).a $(OBJ)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) build
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf $(TARGET) $(TEST) $(BUILDDIR) $(LIB)

build:
	mkdir $(BUILDDIR)

upload:
	rsync -avz . dgzc36@hamilton8.dur.ac.uk:polycube_growth --exclude-from=./exclude --delete