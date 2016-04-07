CC=g++
FLAGS=-std=c++0x
SOURCES_DIR = ../../../src
SOURCES=$(filter-out $(SOURCES_DIR)/main.cpp, $(wildcard $(SOURCES_DIR)/*.cpp)) \
		main.cpp 
INCLUDE=-I$(SOURCES_DIR)
BIN=nestest

all: $(SOURCES) $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(FLAGS) $(INCLUDE) $(SOURCES) -o $@

run:
	./$(BIN)

clean:
	rm -f *.o $(BIN) *.h~ *.cpp~