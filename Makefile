
OPTS= -g -Wuninitialized -Wmaybe-uninitialized -Wall -Wshadow -Wcast-qual \
      -Wextra -pedantic -Wno-unused-parameter \
      -Wno-c++11-extensions \
      -DM2_ENABLE_PROFILE -DM2_ENABLE_PE -DM2_ENABLE_IDREQ

SOURCES=$(shell find ./src -name *.cpp)

OBJECTS=$(SOURCES:./src/%.cpp=./build/$(notdir %).o)

all: dirs $(OBJECTS) midi2

dirs:
	mkdir -p build

build/%.o: src/%.cpp
	gcc $(OPTS) -I . -I ./include \
		-o $@ -c $< 

midi2: $(OBJECTS)
	ar -rc build/libmidi2.a $(OBJECTS)

clean:
	rm -rf build
