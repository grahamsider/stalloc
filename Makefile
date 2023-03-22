CXX       := g++
CXXFLAGS  := -pedantic-errors -Wall -Wextra -Werror
TARGETS   := implist
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
