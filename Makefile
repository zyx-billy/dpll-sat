CXX = g++
CXXFLAGS = -std=c++11

all: parser

parser: main.o tseitin.o parser.o
	$(CXX) $(CXXFLAGS) -o parser main.o tseitin.o parser.o

main.o: main.cpp formula.h parser.h cnf.h tseitin.h
	$(CXX) $(CXXFLAGS) -c main.cpp

tseitin.o: tseitin.cpp formula.h cnf.h parser.h tseitin.h
	$(CXX) $(CXXFLAGS) -c tseitin.cpp

parser.o: parser.cpp formula.h parser.h
	$(CXX) $(CXXFLAGS) -c parser.cpp


clean:
	rm *.o parser
