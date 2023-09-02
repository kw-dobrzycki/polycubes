CC = g++
TARGET = main
TEST = test
LIB = libpolycube.a
CFLAGS = -I$(HDIR) -std=c++2a -O3 -fopenmp

# directories for library, this program is only main.
SRCDIR = src
HDIR = include
OBJDIR = build

SRC := $(wildcard $(SRCDIR)/*.cpp)
OBJ := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
HEADERS := $(wildcard $(HDIR)/*.h) $(wildcard $(SRCDIR)/*.h)

.PHONY: all test library clean

all: main.cpp library
	$(CC) -o $(TARGET) $< -L. -lpolycube $(CFLAGS)

library: $(OBJ)
	ar rcs -o $(LIB) $(OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) build
	$(CC) -c -o $@ $< $(CFLAGS)

test: tests/test.cpp library
	$(CC) -o $@ $< -L. -l$(LIB) $(CFLAGS)
	./$@

clean:
	rm -rf $(TARGET) $(TEST) $(OBJDIR) $(LIB)

build:
	mkdir $(OBJDIR)