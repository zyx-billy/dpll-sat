CXX = g++
CXXFLAGS = -std=c++11

SRC_DIR = ./src
BUILD_DIR = ./build
BIN_DIR = ./bin
DIR_GUARD = @mkdir -p $(@D)

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRC_NAMES = $(notdir $(SRCS))
OBJ_NAMES = $(SRC_NAMES:.cpp=.o)
OBJS = $(addprefix $(BUILD_DIR)/,$(OBJ_NAMES))
DEPS = $(OBJS:.o=.d)

.PHONY: clean

all: $(BIN_DIR)/main

$(BIN_DIR)/main: $(OBJS)
	$(DIR_GUARD)
	$(CXX) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(DIR_GUARD)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

-include $(DEPS)


clean:
	$(RM) $(BUILD_DIR)/* $(BIN_DIR)/*
