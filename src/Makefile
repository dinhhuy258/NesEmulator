CC=g++
FLAGS=-std=c++0x
SOURCES=main.cpp \
		CPU.cpp \
		MemoryCPU.cpp \
		PPU.cpp \
		MemoryPPU.cpp \
		Cartridge.cpp \
		Mapper.cpp \
		Mapper0.cpp 
BIN=NesEmulator

all: $(SOURCES) $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(FLAGS) $(SOURCES) -o $@

run:
	./$(BIN)

clean:
	rm -f *.o $(BIN) *.h~ *.cpp~