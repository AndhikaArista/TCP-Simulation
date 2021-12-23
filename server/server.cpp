#include <iostream>
#include <fstream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

int main()
{
	// Initialize Winsock
	WSADATA wsData; // WinSock data-structure
	WORD ver = MAKEWORD(2, 2); // means 2.2 version

	int wsOk = WSAStartup(ver, &wsData); // initializing Winsock lib 
	if (wsOk != 0)
	{
		cerr << "Can`t Initialize Winsock! Quitting" << endl;
		return 1;
	}

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0); // make socket with IPv4, TCP (SOCK_DGRAM:UDP)
	if (listening == INVALID_SOCKET)
	{
		cerr << "Can`t create a socket! Quitting" << endl;
		return 1;
	}

	// Bind thd socket to an ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000); // host byte to network byte (little->big endian)
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening
	listen(listening, SOMAXCONN);

	fd_set master; // data-structrue that contains list of socket, and their stats
	FD_ZERO(&master);

	FD_SET(listening, &master);

	bool running = true;

	while (running)
	{
		// select function cause chaning of fd. so we need copy of fd
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		for (int i = 0; i < socketCount; i++)
		{
			SOCKET sock = copy.fd_array[i];

			if (sock == listening)
			{
				// Accept a new conneciton
				SOCKET client = accept(listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				// Send a welcome message to the connted client
				string welcomeMsg = "Welcome to the Chat Server!\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);

				// broadcast welcome message
				cout << "Server Ready\n";
				ostringstream ss;
				ss << "SOCKET #" << client << " connect to chat server.\r\n";
				string strOut = ss.str();
				for (int i = 0; i < master.fd_count; i++)
				{
					SOCKET outSock = master.fd_array[i];
					if (outSock != listening && outSock != client)
					{
						send(outSock, strOut.c_str(), strOut.size() + 1, 0);
					}
				}
				// logging welcome message to server
				cout << strOut << endl;
			}
			else
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive Message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					// Check to see if it`s a command.  \quit kills the server
					if (buf[0] == '\\')
					{
						string cmd = string(buf, bytesIn);
						if (cmd == "\\quit")
						{
							running = false;
							break;
						}
					}

					// Send message to other client, and definiately not the listening socket
					ostringstream ss;
					ss << "SOCKET #" << sock << ":" << buf << "\r\n";
					string strOut = ss.str();
					for (int i = 0; i < master.fd_count; i++)
					{
						SOCKET outSock = master.fd_array[i];
						if (outSock != listening)
						{
							send(outSock, strOut.c_str(), strOut.size() + 1, 0);
						}
					}
					// logging user`s message to server
					cout << strOut << endl;
					ifstream inputData;
					ofstream outputData;
					inputData.open("SimpanChat.txt");
					outputData.open("SimpanChat.txt", ios::app);

					if (inputData.is_open())
						outputData << strOut;
					inputData.close();
					outputData.close();
				}
			}
		}
	}
	// Close listening socket
	FD_CLR(listening, &master);
	closesocket(listening);

	// broadcast closing messasge and close clinet socket
	string msg = "Server is shutting down. Goodbye\r\n";
	while (master.fd_count > 0)
	{
		SOCKET sock = master.fd_array[0];

		send(sock, msg.c_str(), msg.size() + 1, 0);

		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup Winsock
	WSACleanup();
	return 0;
}