.SUFFIXES: .cpp .h .o

AR = ar
ARFLAGS += -T

CC = g++
CPPFLAGS += -g -fPIC -I $(INCDIR) -O2 -Wall -std=c++17 

SRCDIR = src
OBJDIR = obj
INCDIR = include
LIBDIR = lib

LIB_NAME = bpt

# main source file
TARGET_SRC:=main.cpp
TARGET_OBJ:=main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=\
	bpt.cpp\
	utils.cpp\
	disk_manager.cpp\
	buffer_manager.cpp\
	page.cpp\
	find.cpp\
	insert.cpp\
	delete.cpp\
	join.cpp

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

TARGET_SRC := $(addprefix $(SRCDIR)/, $(TARGET_SRC))
TARGET_OBJ := $(addprefix $(OBJDIR)/, $(TARGET_OBJ))
SRCS_FOR_LIB := $(addprefix $(SRCDIR)/, $(SRCS_FOR_LIB))
OBJS_FOR_LIB := $(addprefix $(OBJDIR)/, $(OBJS_FOR_LIB))

TARGET = main

.PHONY: all clean obj_dir lib_dir library static_library

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp obj_dir
	$(CC) $(CPPFLAGS) -c -o $@ $<

$(TARGET): $(TARGET_OBJ)
	make static_library
	$(CC) $(CPPFLAGS) -o $@ $< -L $(LIBDIR) -l$(LIB_NAME) -lpthread

clean:
	rm -rf $(TARGET) $(OBJDIR) $(LIBDIR)

obj_dir:
	mkdir -p $(OBJDIR)

lib_dir:
	mkdir -p $(LIBDIR)

library: $(OBJS_FOR_LIB) lib_dir
	$(CC) $(CPPFLAGS) -shared -Wl,-soname,lib$(LIB_NAME).so -o $(LIBDIR)/lib$(LIB_NAME).so $(OBJS_FOR_LIB)

static_library: $(OBJS_FOR_LIB) lib_dir
	$(AR) $(ARFLAGS) $(LIBDIR)/lib$(LIB_NAME).a $(OBJS_FOR_LIB)