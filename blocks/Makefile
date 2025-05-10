#Variables declaration
TARGET_NAME = program.out
IDIR = -Ilib
CXX = g++
CXXFLAGS = -fPIC $(IDIR)
OS_LIBS = -lpthread -lm

ODIR ?= build

#Source and Object Files
SRCS := $(subst ./,,$(shell find . -maxdepth 1 -name "*.cpp"))
OBJS := $(addprefix $(ODIR)/,$(patsubst %.cpp,%.o,$(SRCS)))

#Shared library build
LIB := block_funcs
LIB_SRC := block_funcs.cpp
LIBPATH := $(ODIR)/lib$(LIB).so

#Build shared library
$(LIBPATH): $(LIB_SRC)
	mkdir -p $(ODIR)
	$(CXX) -shared $(CXXFLAGS) -o $@ $<

#Compile each .cpp file into .o files
$(ODIR)/%.o: %.cpp
	mkdir -p $(ODIR)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

#Link all objects + shared lib into one executable
$(ODIR)/$(TARGET_NAME): $(OBJS) $(LIBPATH)
	$(CXX) -o $@ $^ -L$(ODIR) -l$(LIB) $(OS_LIBS)

#Default target
myprog: $(ODIR)/$(TARGET_NAME)

#Clean target
.PHONY: clean
clean:
	rm -rf $(ODIR)
