all: Server Client

Server: Server/Server.c Server/Server.h
	gcc -o server Server/Server.c -pthread

Client: Client/Client.c Client/Client.h
	gcc -o client Client/Client.c -pthread