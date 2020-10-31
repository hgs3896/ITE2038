.SUFFIXES: .c .h .o

AR := ar
ARFLAGS += -T

CC := gcc
CFLAGS += -g -fPIC -I $(INCDIR) -O2 -Wall

SRCDIR := src
OBJDIR := obj
INCDIR := include
LIBDIR := lib

LIB_NAME := bpt

# Main Source File
TARGET_SRC := main.c
TARGET_OBJ := $(TARGET_SRC:.c=.o)

# Include more source files as you need.
SRCS_FOR_LIB := bpt.c

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

TARGET_SRC := $(addprefix $(SRCDIR)/, $(TARGET_SRC))
TARGET_OBJ := $(addprefix $(OBJDIR)/, $(TARGET_OBJ))
SRCS_FOR_LIB := $(addprefix $(SRCDIR)/, $(SRCS_FOR_LIB))
OBJS_FOR_LIB := $(addprefix $(OBJDIR)/, $(OBJS_FOR_LIB))

TARGET = main

.PHONY: all clean obj_dir lib_dir library static_library

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c obj_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(TARGET_OBJ)
	make static_library
	$(CC) $(CFLAGS) -o $@ $< -L $(LIBDIR) -l$(LIB_NAME)

clean:
	rm -rf $(TARGET) $(OBJDIR) $(LIBDIR)

obj_dir:
	mkdir -p $(OBJDIR)

lib_dir:
	mkdir -p $(LIBDIR)

library: $(OBJS_FOR_LIB) lib_dir
	$(CC) $(CFLAGS) -shared -Wl,-soname,lib$(LIB_NAME).so -o $(LIBDIR)/lib$(LIB_NAME).so $(OBJS_FOR_LIB)

static_library: $(OBJS_FOR_LIB) lib_dir
	$(AR) $(ARFLAGS) $(LIBDIR)/lib$(LIB_NAME).a $(OBJS_FOR_LIB)