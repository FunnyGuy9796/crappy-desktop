CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++1y
CXXFLAGS += `pkg-config --cflags x11`
LDFLAGS += `pkg-config --libs x11`

all: crappy_wm

HEADERS = \
    cwm.hpp
SOURCES = \
    cwm.cpp
OBJECTS = $(SOURCES:.cpp=.o)

crappy_wm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f crappy_wm $(OBJECTS)