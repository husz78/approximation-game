CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS = 

# Source files
SERVER_SOURCES = approx-server.cpp server-utils.cpp err.cpp common.cpp
CLIENT_SOURCES = approx-client.cpp client-utils.cpp err.cpp common.cpp

# Header files
HEADERS = err.h common.h server-utils.h client-utils.h

# Object files
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# Executables
SERVER_TARGET = approx-server
CLIENT_TARGET = approx-client

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Server executable
$(SERVER_TARGET): $(SERVER_OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SERVER_OBJECTS) $(LDFLAGS)

# Client executable
$(CLIENT_TARGET): $(CLIENT_OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(CLIENT_OBJECTS) $(LDFLAGS)

# Object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o $(SERVER_TARGET) $(CLIENT_TARGET) *.d

.PHONY: all clean