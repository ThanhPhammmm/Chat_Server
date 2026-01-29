CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
INCLUDES = \
	-Iinclude/TCPServer \
	-Iinclude/TCPSession \
	-Iinclude/CommandHandler \
	-Iinclude/MessageHandler \
	-Iinclude/Controller \
	-Iinclude/ThreadHandler \
	-Iinclude/MessageThreadHandler \
	-Iinclude/Logger \
	-Iinclude/PublicChatRoom \
	-Iinclude/DataBaseManager \
	-Iinclude/UserManager

RUN_DIR = RunProgram

SERVER_TARGET = $(RUN_DIR)/chat_server
CLIENT_TARGET = $(RUN_DIR)/chat_client
LOGGER_TARGET = ./Record
DATABASE_TARGET = ./DataBase

SERVER_SRCS = \
	source/main.cpp \
	source/TCPServer/*.cpp \
	source/TCPSession/*.cpp \
	source/CommandHandler/*.cpp \
	source/Controller/*.cpp \
	source/MessageHandler/*.cpp \
	source/ThreadHandler/*.cpp \
	source/Logger/*.cpp \
	source/PublicChatRoom/*.cpp \
	source/DataBaseManager/*.cpp \
	source/UserManager/*.cpp

CLIENT_SRCS = source/Client/client.cpp

all: server client

server:
	@mkdir -p $(RUN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SERVER_SRCS) -o $(SERVER_TARGET) -lsqlite3 -lcrypto

client:
	@mkdir -p $(RUN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CLIENT_SRCS) -o $(CLIENT_TARGET)

run-server: server
	./$(SERVER_TARGET)

run-client: client
	./$(CLIENT_TARGET)

clean:
	rm -rf $(SERVER_TARGET) $(CLIENT_TARGET) $(LOGGER_TARGET) $(DATABASE_TARGET)

