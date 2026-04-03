CXX := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -fopenmp

# Headers used by main.cpp and project modules
INCLUDES := -I. -I./database_management -I./stocastic_simulation

# PostgreSQL/PQXX + pthread
LIBS := -lpqxx -lpq -lpthread

SRC := main.cpp \
    stocastic_simulation/stocastic_simulation.cpp \
    database_management/database_manager.cpp

BIN := server

# Optional engine test target (kept for convenience)
TEST_SRC := tests/enginetest/enginetest.cpp \
         stocastic_simulation/stocastic_simulation.cpp \
         database_management/database_manager.cpp
TEST_BIN := tests/enginetest/enginetest

.PHONY: all run test clean

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LIBS) -o $@

run: $(BIN)
	./$(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LIBS) -o $@

clean:
	rm -f $(BIN) $(TEST_BIN)