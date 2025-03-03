CXX = g++
CXXFLAGS = -std=c++11 -Wall -g
LDFLAGS = -pthread

SRCS = main.cpp process.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = causal_broadcast

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean