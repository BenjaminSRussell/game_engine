# Makefile for Metal Resource Tests

CC = clang
FRAMEWORKS = -framework Metal -framework Foundation
CFLAGS = -Wall -Wextra -I.. -g
SOURCES = mtl_resource_tests.c \
          ../mtl_device.c \
          ../mtl_buffer.c \
          ../mtl_texture.c \
          ../mtl_transfer.c \
          ../mtl_resource_pool.c \
          ../mtl_sampler.c

TARGET = mtl_tests

.PHONY: all clean test benchmark profile

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(FRAMEWORKS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

benchmark: $(TARGET)
	@echo "Running performance benchmarks..."
	./$(TARGET) | grep -A 10 "Benchmark"

profile: $(TARGET)
	@echo "Running leaks analysis..."
	leaks --atExit -- ./$(TARGET)

instruments: $(TARGET)
	instruments -t Leaks -D leaks_trace.trace ./$(TARGET)
	@echo "Trace saved to leaks_trace.trace"

clean:
	rm -f $(TARGET) *.trace

help:
	@echo "Available targets:"
	@echo "  make          - Build test suite"
	@echo "  make test     - Run all tests"
	@echo "  make benchmark- Run performance benchmarks"
	@echo "  make profile  - Check for memory leaks"
	@echo "  make clean    - Remove built files"
