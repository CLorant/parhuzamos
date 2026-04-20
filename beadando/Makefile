# ============================================================
#  Parallel LSB Steganography  –  Makefile (cross-platform)
#  Targets:  all  clean  dirs  gen  bench
# ============================================================

CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -std=c11 \
           -Iinclude/common \
           -Iinclude/openmp \
           -Iinclude/opencl
OMPFLAG  = -fopenmp
OCLFLAG  = -lOpenCL
MATHFLAG = -lm

# ---- Source files -----------------------------------------------
SRC_COMMON = src/common/image_io.c \
             src/common/stego_utils.c \
             src/common/benchmark.c
SRC_OMP    = src/openmp/stego_openmp.c
SRC_OCL    = src/opencl/kernel_loader.c \
             src/opencl/run_cl.c \
             src/opencl/stego_opencl.c
SRC_MAIN   = main.c
SRCS       = $(SRC_COMMON) $(SRC_OMP) $(SRC_OCL) $(SRC_MAIN)

# ---- Output -----------------------------------------------------
TARGET_WINDOWS = stego.exe
TARGET_LINUX   = stego

# ============================================================
#  Platform detection
# ============================================================
ifeq ($(OS), Windows_NT)
    TARGET = $(TARGET_WINDOWS)
    RM     = del /f /q
    RMDIR  = rmdir /s /q
    NULL   = 2>nul
else
    TARGET = $(TARGET_LINUX)
    RM     = rm -f
    RMDIR  = rm -rf
    NULL   = 2>/dev/null
endif

# ============================================================
.PHONY: all clean dirs gen bench

all: dirs $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(OMPFLAG) $^ -o $@ $(OCLFLAG) $(MATHFLAG)

# Create required directories (explicit and cross‑platform)
dirs:
ifeq ($(OS), Windows_NT)
	@if not exist data\results mkdir data\results
	@if not exist data\plots mkdir data\plots
	@if not exist data\outputs mkdir data\outputs
	@if not exist data\samples mkdir data\samples
else
	@mkdir -p data/results data/plots data/outputs data/samples
endif

# Generate a 1024×1024 test carrier
gen: all
	./$(TARGET) gen 1024 1024 data/samples/test_1024.ppm

# Run the full benchmark with default sizes and thread counts
bench: all
	./$(TARGET) bench

clean:
ifeq ($(OS), Windows_NT)
	-$(RM) $(TARGET_WINDOWS) $(NULL)
	-$(RM) data\results\*.csv $(NULL)
	-$(RM) data\plots\*.png $(NULL)
	-$(RM) data\outputs\*.ppm $(NULL)
	-$(RM) data\samples\*.ppm $(NULL)
	-$(RMDIR) data\results data\plots data\outputs data\samples $(NULL)
else
	-$(RM) $(TARGET_LINUX)
	-$(RM) data/results/*.csv
	-$(RM) data/plots/*.png
	-$(RM) data/outputs/*.ppm
	-$(RM) data/samples/*.ppm
	-$(RMDIR) data/results data/plots data/outputs data/samples $(NULL)
endif