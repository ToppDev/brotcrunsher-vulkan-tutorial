CXX		  := g++
CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb
LDFLAGS = `pkg-config --static --libs glfw3` -lvulkan

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

LIBRARIES	:=
EXECUTABLE	:= main


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES) $(LDFLAGS)

.PHONY: test clean

clean:
	-rm $(BIN)/*
