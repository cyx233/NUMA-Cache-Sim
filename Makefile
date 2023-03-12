CC = gcc
CXX = g++ 
CXXFLAGS = -std=c++17 -O0 -Wall -Wextra -Wshadow -Wpedantic
DEPS =  cache_block.h moesi_block.h cache.h directory.h numa_node.h
OBJDIR = build
vpath %.h src
vpath %.cpp src
OBJ = $(addprefix $(OBJDIR)/, msi_block.o moesi_block.o cache.o directory.o numa_node.o)

# Default build rule
.PHONY: all
all:clean bin programs

bin: $(OBJ) $(OBJDIR)/main.o
	$(CXX) $(CXXFLAGS) -o sim.out $(OBJ) $(OBJDIR)/main.o

.PHONY: debug
debug: CXXFLAGS += -DDEBUG -g
debug: all

.PHONY: programs
programs:
	(cd programs && make)

test: $(OBJ) $(OBJDIR)/test.o
	$(CXX) $(CXXFLAGS) -o test.out $(OBJ) $(OBJDIR)/test.o

$(OBJDIR)/%.o: %.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o *.out
	(cd programs && make clean)
	