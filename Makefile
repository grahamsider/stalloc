CXX       := g++
CXXFLAGS  := -std=c++20 -pedantic-errors -Wall -Wextra -Werror -O2
TARGETS   := implist explist seglist
SRC_DIR   := ./src
BUILD_DIR := ./build

all: $(TARGETS)

debug: CXXFLAGS += -DDEBUG -g
debug: all

$(TARGETS):
	@mkdir -p $(BUILD_DIR)/$@
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/$@/*.cpp -o $(BUILD_DIR)/$@/$@_test

.PHONY: clean

clean:
	@rm -rf $(BUILD_DIR)
