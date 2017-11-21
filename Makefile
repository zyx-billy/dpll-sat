CXX = g++
CXXFLAGS = -std=c++11

all: parser

parser: parser.o
	$(CXX) $(CXXFLAGS) -o parser parser.o

parser.o: parser.cpp parser.h formula.h
	$(CXX) $(CXXFLAGS) -c parser.cpp


clean:
	rm *.o parser
