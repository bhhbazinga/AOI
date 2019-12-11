CXX = g++
CXXFLAGS = -Wall -Werror=return-type -Wextra -std=c++17 -g -O3
# -fsanitize=address
EXEC = test

all: $(EXEC)

$(EXEC): test.cc crosslink_aoi.o quadtree_aoi.o tower_aoi.o
	$(CXX) $(CXXFLAGS) -o $(EXEC) test.cc crosslink_aoi.o quadtree_aoi.o tower_aoi.o -I./

crosslink_aoi.o:crosslink_aoi/crosslink_aoi.cc crosslink_aoi/crosslink_aoi.h  aoi.h
	$(CXX) $(CXXFLAGS) -o crosslink_aoi.o -c crosslink_aoi/crosslink_aoi.cc -I./

quadtree_aoi.o:quadtree_aoi/quadtree_aoi.cc quadtree_aoi/quadtree_aoi.h  aoi.h
	$(CXX) $(CXXFLAGS) -o quadtree_aoi.o -c quadtree_aoi/quadtree_aoi.cc -I./

tower_aoi.o:tower_aoi/tower_aoi.cc tower_aoi/tower_aoi.h  aoi.h
	$(CXX) $(CXXFLAGS) -o tower_aoi.o -c tower_aoi/tower_aoi.cc -I./

.Phony: clean

clean:
	rm -rf *.o $(EXEC)