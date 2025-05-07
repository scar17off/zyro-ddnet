#include "gui.h"
#include "game/client/gameclient.h"

#include <cstring> // For memset
#include <iostream>
#include <string>
#include <sys/types.h>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSE_SOCKET(s) close(s)
#endif

void CGui::RunServer()
{
	const int PORT = 12345; // Port to listen on
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

#ifdef _WIN32
	// Initialize Winsock
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cerr << "WSAStartup failed" << std::endl;
		return;
	}
#endif

	// Create socket file descriptor
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
#ifdef _WIN32
		std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
#else
		perror("Socket creation failed");
#endif
		return;
	}

	// Forcefully attach socket to the port
	if(
#ifdef _WIN32
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))
#else
		setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))
#endif
	)
	{
#ifdef _WIN32
		std::cerr << "Setsockopt failed: " << WSAGetLastError() << std::endl;
#else
		perror("Setsockopt failed");
#endif
		CLOSE_SOCKET(server_fd);
		return;
	}

	// Configure the address structure
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// Bind the socket to the address
	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
#ifdef _WIN32
		std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
#else
		perror("Bind failed");
#endif
		CLOSE_SOCKET(server_fd);
		return;
	}

	// Start listening for connections
	if(listen(server_fd, 3) < 0)
	{
#ifdef _WIN32
		std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
#else
		perror("Listen failed");
#endif
		CLOSE_SOCKET(server_fd);
		return;
	}

	std::cout << "Server listening on port " << PORT << std::endl;

	// Accept incoming connections
	while(true)
	{
		if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
		{
#ifdef _WIN32
			std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
#else
			perror("Accept failed");
#endif
			CLOSE_SOCKET(server_fd);
			return;
		}

		std::cout << "New connection established" << std::endl;

		// Handle client communication in a separate thread
		std::thread([new_socket]() {
			char buffer[1024] = {0};
			const char *welcome_message = "Welcome to the server!";
			send(new_socket, welcome_message, strlen(welcome_message), 0);

			while(true)
			{
				memset(buffer, 0, sizeof(buffer));
				int bytes_read =
#ifdef _WIN32
					recv(new_socket, buffer, sizeof(buffer) - 1, 0);
#else
					read(new_socket, buffer, sizeof(buffer) - 1);
#endif
				if(bytes_read <= 0)
				{
					std::cout << "Client disconnected" << std::endl;
					CLOSE_SOCKET(new_socket);
					break;
				}

				std::cout << "Received: " << buffer << std::endl;

				// Echo the message back to the client
				send(new_socket, buffer, bytes_read, 0);
			}
		}).detach();
	}

#ifdef _WIN32
	WSACleanup();
#endif
}