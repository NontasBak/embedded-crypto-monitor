# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Wall -I./src

# Libraries to link
LIBS = -lwebsockets -lpthread

# Source files
SOURCES = src/main.cpp \
          src/websocket/okx_client.cpp \
          src/scheduler/scheduler.cpp \
          src/utils/setup.cpp \
          src/measurement/measurement.cpp \
          src/moving_average/moving_average.cpp \
          src/pearson/pearson.cpp

# Name of executable
TARGET = crypto_monitor

# The main build rule
all: $(TARGET)

# How to build the executable
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

# Clean up
clean:
	rm -f $(TARGET)

# Run the program
run: all
	./$(TARGET)

# Phony targets
.PHONY: all clean run
