SOURCES = src/main.cpp \
          src/websocket/okx_client.cpp \
          src/scheduler/scheduler.cpp \
          src/utils/setup.cpp \
          src/utils/cpu_stats.cpp \
          src/measurement/measurement.cpp \
          src/data_collector/data_collector.cpp \
          src/pearson/pearson.cpp \
          src/server/server.cpp

LIBS = -lwebsockets -lpthread

TARGET = crypto_monitor
CXX = g++
CXXFLAGS = -std=c++14 -Wall -I./src

TARGET_RPI = crypto_monitor_rpi
CROSS_PREFIX = aarch64-linux-gnu-
SYSROOT = /home/nontas/sysroot-rpi

CXX_RPI = $(CROSS_PREFIX)g++
CXXFLAGS_RPI = -std=c++14 -Wall --sysroot=$(SYSROOT) -I./src -I$(SYSROOT)/usr/include
LDFLAGS_RPI = --sysroot=$(SYSROOT)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

$(TARGET_RPI): $(SOURCES)
	$(CXX_RPI) $(CXXFLAGS_RPI) $(SOURCES) -o $(TARGET_RPI) $(LDFLAGS_RPI) $(LIBS)

rpi: $(TARGET_RPI)

clean:
	rm -f $(TARGET) $(TARGET_RPI)

run: all
	./$(TARGET)

.PHONY: all rpi clean run deploy
