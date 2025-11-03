# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -std=c99 -O2
LIBS = -lm

# Program name
TARGET = bitrate_scope

# Source files
SOURCES = ffmpeg_microscope.c

# Default target
all: $(TARGET)

# Build the main program
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)
	@echo "Built $(TARGET) successfully!"

# Build with debug information
debug: $(SOURCES)
	$(CC) $(CFLAGS) -g -DDEBUG $(SOURCES) -o $(TARGET)_debug $(LIBS)
	@echo "Built debug version successfully!"

# Build for profiling
profile: $(SOURCES)
	$(CC) $(CFLAGS) -pg $(SOURCES) -o $(TARGET)_profile $(LIBS)
	@echo "Built profile version successfully!"

# Clean up build files
clean:
	rm -f $(TARGET) $(TARGET)_debug $(TARGET)_profile
	rm -f *.o
	rm -f gmon.out
	rm -f *~
	@echo "Cleaned up build files!"

# Install the program
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "Installed $(TARGET) to /usr/local/bin/"

# Uninstall the program
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET) from /usr/local/bin/"

# Run tests
test: $(TARGET)
	@if [ -f test_data.csv ]; then \
		./$(TARGET) test_data.csv test_output.txt; \
		echo "Test completed!"; \
	else \
		echo "No test_data.csv found"; \
	fi

# Create test data
create-test-data:
	@echo "Creating test data..."
	@echo "chunk_index,bitrate_per_chunk,frame_count,chunk_size" > test_data.csv
	@for i in 0 1 2 3 4 5 6 7 8 9 10; do \
		echo "$$i,5000,25,180000" >> test_data.csv; \
	done
	@echo "Created test_data.csv"

# Help message
help:
	@echo "Available commands:"
	@echo "  make              - Build the program"
	@echo "  make debug        - Build with debug info"
	@echo "  make profile      - Build for profiling"
	@echo "  make clean        - Clean build files"
	@echo "  make install      - Install program"
	@echo "  make uninstall    - Uninstall program"
	@echo "  make test         - Run tests"
	@echo "  make create-test-data - Create test file"
	@echo "  make help         - Show this help"

.PHONY: all debug profile clean install uninstall test create-test-data help
