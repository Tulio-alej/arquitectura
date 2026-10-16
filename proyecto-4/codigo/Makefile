CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude -Ilibrerias-lpc
BUILD    := build

LIB_SRC  := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
LIB_OBJ  := $(LIB_SRC:src/%.cpp=$(BUILD)/%.o)
BIN      := $(BUILD)/cachesim
TEST_BIN := $(BUILD)/tests

.PHONY: all run benchmark test demo didactico sweep clean ayuda

all: $(BIN)

$(BUILD)/%.o: src/%.cpp $(wildcard include/*.hpp) $(wildcard librerias-lpc/*.hpp) | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN): $(LIB_OBJ) $(BUILD)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_BIN): tests/test_simulador.cpp $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD):
	mkdir -p $(BUILD)

run: $(BIN)
	./$(BIN) compare --csv results_v2.csv

benchmark: run

test: $(TEST_BIN)
	./$(TEST_BIN)

demo: $(BIN)
	./$(BIN) compare --workload zipf --length 50000

didactico: $(BIN)
	./$(BIN) didactico --size 256 --block 64 --ways 2 --file trazas/ejemplo_simple.trace --pasos 10

sweep: $(BIN)
	./$(BIN) sweep --param ways --workload caliente_frio

ayuda: $(BIN)
	./$(BIN) ayuda

clean:
	rm -rf $(BUILD) results_v2.csv
