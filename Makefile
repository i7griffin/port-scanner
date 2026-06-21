CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17

build/scanner: src/main.cpp
	$(CXX) $(CXXFLAGS) -o build/scanner src/main.cpp

clean:
	rm -f build/scanner
