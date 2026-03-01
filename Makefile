CXX = g++

CXXFLAGS = -std=c++11 -Wall -Wextra -O2

TARGET = myls

SRCS = myls.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)
