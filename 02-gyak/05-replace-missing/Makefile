CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -lOpenCL -lm -Iinclude

TARGET  = replace_missing
SRCS    = main.c src/replace_missing.c src/run_cl.c src/kernel_loader.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean