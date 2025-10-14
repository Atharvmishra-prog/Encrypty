CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread -I/opt/homebrew/include
LDFLAGS := -L/opt/homebrew/lib -lssl -lcrypto

SRCS := main-mmap.cpp ./utils/ThreadPool/thread_pool.cpp

OBJS := $(SRCS:.cpp=.o)

EXES := mmap

all: $(EXES)

mmap: main-mmap.o ./utils/ThreadPool/thread_pool.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXES)

.PHONY: all clean
