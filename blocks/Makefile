#Variables declaration
TARGET_NAME = program.out
IDIR = -Ilib
CXX = g++
CXXFLAGS = -fPIC $(IDIR)
OS_LIBS = -lpthread -lm

ODIR ?= build

#Source and Object Files
SRCS := $(filter-out block_funcs.cpp, $(subst ./,,$(shell find . -maxdepth 1 -name "*.cpp")))
OBJS := $(addprefix $(ODIR)/,$(patsubst %.cpp,%.o,$(SRCS)))
OUTS := $(addprefix $(ODIR)/,$(patsubst %.cpp,%.out,$(SRCS)))

#Shared library build
LIB := block_funcs
LIB_SRC := block_funcs.cpp
LIBPATH := $(ODIR)/lib$(LIB).so

# Default target
all: $(OUTS) $(LIBPATH)

#Build shared library
$(LIBPATH): $(LIB_SRC)
	mkdir -p $(ODIR)
	$(CXX) -shared $(CXXFLAGS) -o $@ $<

#Build .out files
$(ODIR)/%.out: %.cpp block_funcs.cpp block_funcs.h
	mkdir -p $(ODIR)
	$(CXX) $(CXXFLAGS) $< block_funcs.cpp -o $@

#Link all objects + shared lib into one executable
$(ODIR)/$(TARGET_NAME): $(OBJS) $(LIBPATH)
	$(CXX) -o $@ $^ -L$(ODIR) -l$(LIB) $(OS_LIBS)

#Default target
myprog: $(ODIR)/$(TARGET_NAME)

#Clean target
.PHONY: clean
clean:
	rm -rf $(ODIR)
