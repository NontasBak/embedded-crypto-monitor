# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++14 -Wall -I./src

# For debugging, use this
# CXXFLAGS = -std=c++14 -Wall -I./src -g -O0

# Libraries to link
LIBS = -lwebsockets -lpthread -lcpp-httplib

# Source files
SOURCES = src/main.cpp \
          src/websocket/okx_client.cpp \
          src/scheduler/scheduler.cpp \
          src/utils/setup.cpp \
          src/utils/cpu_stats.cpp \
          src/measurement/measurement.cpp \
          src/data_collector/data_collector.cpp \
          src/pearson/pearson.cpp \
          src/server/server.cpp

# Name of executable
TARGET = crypto_monitor

# The main build rule
all: $(TARGET)

# Debug target
# debug: CXXFLAGS += -DDEBUG
# debug: $(TARGET)

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
