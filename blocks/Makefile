#Variables declaration
IDIR = -Ilib
CXX = g++
CXXFLAGS = -fPIC $(IDIR)
OS_LIBS = -lpthread -lm

ODIR ?= build
ABS_ODIR := $(abspath $(ODIR))
LDFLAGS := -L$(ODIR) -lblock_funcs -Wl,-rpath,$(ABS_ODIR) $(OS_LIBS)

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
$(ODIR)/%.out: %.cpp block_funcs.h $(LIBPATH)
	mkdir -p $(ODIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

#Clean target
.PHONY: clean
clean:
	rm -rf $(ODIR)
