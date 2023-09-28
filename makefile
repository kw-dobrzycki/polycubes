CC = g++
TARGET = cubes
TEST = test
LIB = libpolycube.a
CFLAGS = -I$(HDIR) -std=c++2a -O3 -fopenmp

# directories for library, this program is only main.
SRCDIR = src
HDIR = include
OBJDIR = build

#exclude files
EXCLUDE_HEADERS :=
EXCLUDE_SOURCES :=

SRC := $(wildcard $(SRCDIR)/*.cpp)
FILTER_SRC := $(filter-out $(patsubst %, $(SRCDIR)/%.cpp, $(EXCLUDE_SOURCES)), $(SRC))
OBJ := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(FILTER_SRC))

HEADERS := $(wildcard $(HDIR)/*.h) $(wildcard $(SRCDIR)/*.h)
FILTER_HEADERS := $(filter-out $(patsubst %, $(HDIR)/%.h, $(EXCLUDE_HEADERS)), $(HEADERS))

.PHONY: all test library clean

all: main.cpp library
	$(CC) -o $(TARGET) $< -L. -lpolycube $(CFLAGS)

library: $(OBJ)
	ar rcs -o $(LIB) $(OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(FILTER_HEADERS) build
	$(CC) -c -o $@ $< $(CFLAGS)

test: tests/test.cpp library
	$(CC) -o $@ $< -L. -l$(LIB) $(CFLAGS)
	./$@

clean:
	rm -rf $(TARGET) $(TEST) $(OBJDIR) $(LIB)

build:
	mkdir $(OBJDIR)