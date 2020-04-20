CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -Werror -std=c++17 -O3
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread -lboost_thread -lboost_system
OBJ=$(SRC:.cc=.o)

all:  cache_server workload_generator

cache_server: cache_server.o cache_lib.o fifo_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

workload_generator: workload_generator.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf *.o cache_server workload_generator

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./cache_server
	valgrind --leak-check=full --show-leak-kinds=all ./workload_generator
