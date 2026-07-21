# ssdsim linux support

CC = gcc
SRC_DIR = ./src
OBJ_DIR = ./obj
TARGET = main

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))

OBJS = $(SRCS:.c=.o)

OBJECTS = $(patsubst %.o,$(OBJ_DIR)/%.o,$(OBJS))
DEPS = $(OBJECTS:.o=.d)

all: main

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -Wno-format -Wno-implicit-function-declaration -Wno-implicit-int -c -g $< -o $@ -MD

$(TARGET) : $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)
	$(CC) $(OBJECTS) -o debug -DDEBUG

.PHONY: clean all
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET) debug

-include $(DEPS)