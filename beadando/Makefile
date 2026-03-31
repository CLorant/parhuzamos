# Fordító és kapcsolók
CC = gcc
CFLAGS = -Iinclude/ -Wall -Wextra -O3 -fopenmp -g
LDFLAGS = -fopenmp
LIBS_WINDOWS = -lmingw32
TARGET_WINDOWS = maze.exe
TARGET_LINUX = maze


# Forrásfájlok és objektumfájlok
SRC_DIR = src
OBJ_DIR = obj
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Alapértelmezett cél: operációs rendszer detektálása
ifeq ($(OS), Windows_NT)
    TARGET = $(TARGET_WINDOWS)
    LIBS = $(LIBS_WINDOWS)
    MKDIR = if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
    RM = del /f /q
    RMDIR = rmdir /s /q
else
    TARGET = $(TARGET_LINUX)
    LIBS =
    MKDIR = mkdir -p $(OBJ_DIR)
    RM = rm -f
    RMDIR = rm -rf
endif

all: $(TARGET)

# Windows cél
windows: $(TARGET_WINDOWS)

$(TARGET_WINDOWS): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $@ $(LDFLAGS) $(LIBS_WINDOWS)

# Linux cél
linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Objektumfájlok fordítása
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Tisztítás
clean:
ifeq ($(OS), Windows_NT)
	-$(RM) $(OBJ_DIR)\*.o
	-$(RM) $(TARGET_WINDOWS)
	-$(RMDIR) $(OBJ_DIR)
else
	$(RM) $(OBJ_DIR)/*.o
	$(RM) $(TARGET_LINUX)
	$(RMDIR) $(OBJ_DIR)
endif

.PHONY: all windows linux clean