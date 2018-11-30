.SUFFIXES: .cpp .o
CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=\
	$(SRCDIR)bpt.cpp\
	$(SRCDIR)utils.cpp\
	$(SRCDIR)disk_manager.cpp\
	$(SRCDIR)buffer_manager.cpp\
	$(SRCDIR)page.cpp\
	$(SRCDIR)find.cpp\
	$(SRCDIR)insert.cpp\
	$(SRCDIR)delete.cpp\
	$(SRCDIR)join.cpp	

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)
CPPFLAGS+= -g -fPIC -I $(INC) -std=c++17 -gdwarf-2

TARGET=main

all: $(TARGET)
$(TARGET): $(TARGET_OBJ)
	$(foreach file,$(SRCS_FOR_LIB),$(CC) $(CPPFLAGS) -o $(file:.cpp=.o) -c $(file);)
	make static_library
	$(CC) $(CPPFLAGS) -o $@ $^ -L $(LIBS) -lbpt -lpthread

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

library:
	$(CC) -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar rcus $(LIBS)libbpt.a $(OBJS_FOR_LIB)