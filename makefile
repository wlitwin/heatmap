LIBS := $(shell pkg-config --libs opencv)

CXX := g++
CXX_FLAGS := -Wall -Wextra -pedantic -std=c++03 -O2 -flto

NAME := heatmap

SOURCES := utils.cpp

all:
	$(CXX) $(CXX_FLAGS) $(NAME).cpp $(SOURCES) $(LIBS) -o $(NAME)
