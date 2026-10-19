CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -pthread
LDFLAGS = -pthread

APP = battleship
APP_SRC = src/main.c src/engine.c src/workers.c src/ui.c
TEST_BIN = tests/test_engine

.PHONY: all test clean

all: $(APP)

$(APP): $(APP_SRC) include/game.h include/engine.h include/workers.h include/ui.h
	$(CC) $(CFLAGS) $(APP_SRC) -o $@ $(LDFLAGS)

$(TEST_BIN): tests/test_engine.c src/engine.c include/game.h include/engine.h
	$(CC) $(CFLAGS) tests/test_engine.c src/engine.c -o $@ $(LDFLAGS)

test: $(APP) $(TEST_BIN)
	./$(TEST_BIN)
	sh tests/test_game.sh

clean:
	rm -f $(APP) $(TEST_BIN)
