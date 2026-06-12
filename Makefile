CC = gcc
CFLAGS = -pedantic -Wall -Wextra -std=c90 -m64 -O2
LDFLAGS = -ldl -lm 

TARGET = gbstyle

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:$(OBJ_DIR)/%.o=$(OBJ_DIR)/%.d)

$(BIN_DIR)/$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(OBJS): $(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

$(DEPS): $(OBJ_DIR)/%.d : $(SRC_DIR)/%.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(DEPS)
	rm -f $(BIN_DIR)/$(TARGET)
