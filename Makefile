# ==================== compiler ====================
CXX := g++
STD := -std=c++20
WARN := -Wall -Wextra -Wpedantic

# ==================== targets ====================
BUILD_TARGET := nblast++
TEST_TARGET := test_runner

# ==================== source files ====================
SRC := $(shell find src -name "*.cpp")
TEST_SRC := $(wildcard unit-tests/*.cpp)

BUILD ?= release

ifeq ($(BUILD),debug)
    CXXFLAGS := $(STD) $(WARN) -g -Og -DDEBUG -DLOG -MMD -MP
    OBJ_DIR := obj/debug
else
    CXXFLAGS := $(STD) $(WARN) -O2 -DNDEBUG -MMD -MP
    OBJ_DIR := obj/release
endif

OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRC))
DEPS := $(OBJS:.o=.d)

# ==================== main program ====================
all: $(BUILD_TARGET)

$(BUILD_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ==================== test runner ====================
TEST_SRC_FILTERED := $(filter-out src/app/Main.cpp,$(SRC)) $(TEST_SRC)

$(TEST_TARGET): $(TEST_SRC_FILTERED)
	$(CXX) $(CXXFLAGS) -Isrc -Iunit-tests $^ -o $@

# ==================== object files ====================
$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Isrc -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

# ==================== build modes ====================
debug:
	$(MAKE) BUILD=debug -j8

release:
	$(MAKE) BUILD=release -j8

# ==================== clean ====================
clean:
	rm -rf obj log $(BUILD_TARGET) $(TEST_TARGET)

# ==================== run tests ====================
test: $(TEST_TARGET)
	./$(TEST_TARGET)

-include $(DEPS)

# ==================== phony targets ====================
.PHONY: all debug release clean test
