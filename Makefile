.PHONY: all clean

CXX := g++
CXX_FLAGS := -std=gnu++17 -ggdb3 -O2 -Werror -Wall -Wextra -Wshadow -fPIC -fno-strict-aliasing -fwrapv -pthread -U_FORTIFY_SOURCE

LD := g++

HEADERS := $(wildcard src/*.h)
SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
BIN := build/paraldis

all: prepare $(BIN)

prepare:
	@mkdir -p build

$(BIN): $(OBJS)
	$(LD) -o $@ $^

build/%.o: src/%.cpp $(HEADERS)
	$(CXX) -c -o $@ $(CXX_FLAGS) $<

clean:
	@rm -rf build
