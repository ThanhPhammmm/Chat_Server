CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
INCLUDES = \
	-Iinclude/TCPServer \
	-Iinclude/TCPSession \
	-Iinclude/CommandHandler \
	-Iinclude/MessageHandler \
	-Iinclude/Controller \
	-Iinclude/ThreadHandler \
	-Iinclude/ThreadMessageHandler \
	-Iinclude/Logger

RUN_DIR = RunProgram

SERVER_TARGET = $(RUN_DIR)/chat_server
CLIENT_TARGET = $(RUN_DIR)/chat_client
LOGGER_TARGET = $(RUN_DIR)/Record

SERVER_SRCS = \
	source/main.cpp \
	source/TCPServer/*.cpp \
	source/TCPSession/*.cpp \
	source/CommandHandler/*.cpp \
	source/Controller/*.cpp \
	source/MessageHandler/*.cpp \
	source/ThreadHandler/*.cpp \
	source/Logger/*.cpp

CLIENT_SRCS = source/Client/client.cpp

all: server client

server:
	@mkdir -p $(RUN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SERVER_SRCS) -o $(SERVER_TARGET)

client:
	@mkdir -p $(RUN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CLIENT_SRCS) -o $(CLIENT_TARGET)

run-server: server
	./$(SERVER_TARGET)

run-client: client
	./$(CLIENT_TARGET)

clean:
	rm -rf $(SERVER_TARGET) $(CLIENT_TARGET) $(LOGGER_TARGET)

