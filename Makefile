CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
INCLUDES = \
	-Iinclude \
	-Iinclude/TCPServer \
	-Iinclude/TCPSession

RUN_DIR = RunProgram

SERVER_TARGET = $(RUN_DIR)/chat_server
CLIENT_TARGET = $(RUN_DIR)/chat_client

SERVER_SRCS = \
	source/main.cpp \
	source/TCPServer/TCPServer.cpp \
	source/TCPSession/Epoll.cpp \
	source/TCPSession/ThreadPool.cpp

CLIENT_SRCS = \
	source/Client/client.cpp

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
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

