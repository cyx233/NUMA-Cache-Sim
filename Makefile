CC = gcc
CXX = g++ 
CXXFLAGS = -g -std=c++17 -O0 -Wall -Wextra -Wshadow -Wpedantic
DEPS =  cache_block.h moesi_block.h vi_block.h cache.h
OBJDIR = build
vpath %.h src
vpath %.cpp src
OBJ = $(addprefix $(OBJDIR)/, vi_block.o cache.o)

# Default build rule
.PHONY: all
all: test

sim: $(OBJ) $(OBJDIR)/sim.o
	$(CXX) $(CXXFLAGS) -o sim.out $(OBJ) $(OBJDIR)/sim.o

test: $(OBJ) $(OBJDIR)/test.o
	$(CXX) $(CXXFLAGS) -o test.out $(OBJ) $(OBJDIR)/test.o

$(OBJDIR)/%.o: %.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o sim.out a.out