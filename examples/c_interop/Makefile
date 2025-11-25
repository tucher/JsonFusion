# Makefile for C interop example

CXX = g++
CC = gcc
CXXFLAGS = -std=c++23 -I../../include
CFLAGS = -std=c99

TARGET = c_interop_test
OBJS = main.o parser.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

main.o: main.c structures.h
	$(CC) $(CFLAGS) -c $< -o $@

parser.o: parser.cpp structures.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test

