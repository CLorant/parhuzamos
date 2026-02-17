CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS = -lOpenCL -lm -Iinclude

TARGET  = vector_add
SRCS    = main.c src/vector.c src/run_cl.c src/kernel_loader.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean