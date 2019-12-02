CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g -O3
# -fsanitize=address
EXEC = test

all: $(EXEC)

$(EXEC): test.cc crosslink_aoi.o
	$(CXX) $(CXXFLAGS) -o $(EXEC) test.cc crosslink_aoi/scene.cc crosslink_aoi.o -I./

crosslink_aoi.o:crosslink_aoi/crosslink_aoi.cc crosslink_aoi/crosslink_aoi.h  aoi.h
	$(CXX) $(CXXFLAGS) -o crosslink_aoi.o -c crosslink_aoi/crosslink_aoi.cc -I./

.Phony: clean

clean:
	rm -rf *.o $(EXEC)