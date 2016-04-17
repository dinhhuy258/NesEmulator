CC=g++
FLAGS=-std=c++0x -lGL -lGLU -lglut
FLAGS_DEBUG=-std=c++0x -lGL -lGLU -lglut -g
SOURCES=main.cpp \
		CPU.cpp \
		MemoryCPU.cpp \
		PPU.cpp \
		MemoryPPU.cpp \
		Cartridge.cpp \
		Mapper.cpp \
		Mapper0.cpp \
		Mapper2.cpp \
		Controller.cpp
BIN=NesEmulator

all: clean $(SOURCES) $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(FLAGS)

run:
	./$(BIN)

clean:
	rm -f *.o $(BIN) *.h~ *.cpp~ *.out

debug: 
	$(CC) $(SOURCES) $(FLAGS_DEBUG)

run_debug:
	gdb ./a.out
