CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
DATA_DIR = data

SERVER_SRCS = $(SRC_DIR)/server.c \
              $(SRC_DIR)/admin_ops.c \
              $(SRC_DIR)/faculty_ops.c \
              $(SRC_DIR)/student_ops.c \
              $(SRC_DIR)/auth.c \
              $(SRC_DIR)/comm.c \
              $(SRC_DIR)/utilities.c

CLIENT_SRCS = $(SRC_DIR)/client.c \
              $(SRC_DIR)/auth.c \
              $(SRC_DIR)/comm.c \
              $(SRC_DIR)/utilities.c

.PHONY: all clean

all: server client

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) $^ -o server

client: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $^ -o client

clean:
	rm -f server client
