#include "pch.h"
#include <iostream>
#include <cstdlib>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstring>
#include <string>
#include <thread>
#include <exception>
#include <boost/program_options.hpp>

#pragma comment(lib, "ws2_32.lib")

namespace po = boost::program_options;

using namespace std;

void initWSA()
{
	WORD mWSAver = MAKEWORD(2, 2);
	auto mWSA = WSADATA();
	if (0 != WSAStartup(mWSAver, &mWSA))
	{
		cerr << "WSAStartup() failed\n";
		exit(1);
	}
}

class Server_v4
{
public:
	Server_v4(const char* IP, const int port) :ServerIP(IP), ServerPort(port) {}

	void serveForever() {
		ServerSocket = createServerSocket(ServerIP, ServerPort);
	}

	~Server_v4() { close(); }

	SOCKET ServerSocket;

	struct sockaddr_in ServerAddr;

private:

	int ServerPort;

	const char* ServerIP = nullptr;

	void close()
	{
		int Ret = closesocket(ServerSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	SOCKET createServerSocket(const char* IP, int port)
	{
		const int ServerPort = port;
		SOCKET Server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (Server == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}

		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin_family = AF_INET;
		inet_pton(AF_INET, IP, &ServerAddr.sin_addr);
		ServerAddr.sin_port = htons(ServerPort);
		DWORD TTL = 255;
		int Ret = setsockopt(Server, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<char*>(&TTL), sizeof(TTL));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Server);
			Server = INVALID_SOCKET;
		}
		return Server;
	}

};

class Server_v6
{
public:
	Server_v6(const char* IP, const int port) :ServerIP(IP), ServerPort(port) {}

	void serveForever() {
		ServerSocket = createServerSocket(ServerIP, ServerPort);
	}

	~Server_v6() { close(); }

	SOCKET ServerSocket;

	struct sockaddr_in6 ServerAddr;
private:

	int ServerPort;

	const char* ServerIP = nullptr;

	void close()
	{
		int Ret = closesocket(ServerSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	SOCKET createServerSocket(const char* IP, int port)
	{
		const int ServerPort = port;
		SOCKET Server = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (Server == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}

		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin6_family = AF_INET6;
		inet_pton(AF_INET6, IP, &ServerAddr.sin6_addr);
		ServerAddr.sin6_port = htons(ServerPort);
		DWORD HOPS = 255;
		int Ret = setsockopt(Server, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<char*>(&HOPS), sizeof(HOPS));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Server);
			Server = INVALID_SOCKET;
		}
		return Server;
	}
};

class Client_v4
{
public:
	Client_v4(const char* ADDR, int port)
	{
		ClientSocket = createClientSocket(ADDR, port);
	}

	SOCKET ClientSocket;

	~Client_v4() { close(); }
private:

	void close()
	{
		int Ret = closesocket(ClientSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	SOCKET createClientSocket(const char* ADDR, int port)
	{
		const int ClientPort = port;
		SOCKET Client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (Client == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}
		struct sockaddr_in ClientAddr;
		ZeroMemory(&ClientAddr, sizeof(ClientAddr));
		ClientAddr.sin_family = AF_INET;
		ClientAddr.sin_addr = in4addr_any;
		ClientAddr.sin_port = htons(ClientPort);
		int BindRes = ::bind(Client, (struct sockaddr *)&ClientAddr, sizeof(ClientAddr));
		if (BindRes == SOCKET_ERROR)
		{
			cerr << "bind() failed\n";
			WSACleanup();
			exit(1);
		}

		ip_mreq JoinAddr;
		inet_pton(AF_INET, ADDR, &JoinAddr.imr_multiaddr);
		inet_pton(AF_INET, "0.0.0.0", &JoinAddr.imr_interface);

		int Ret = setsockopt(Client, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char*>(&JoinAddr), sizeof(JoinAddr));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Client);
			Client = INVALID_SOCKET;
		}
		DWORD Timeout = 2000;
		Ret = setsockopt(Client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&Timeout), sizeof(Timeout));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Client);
			Client = INVALID_SOCKET;
		}

		return Client;
	}
};

class Client_v6
{
public:
	Client_v6(const char* ADDR, int port)
	{
		ClientSocket = createClientSocket(ADDR, port);
	}

	~Client_v6() { close(); }

	SOCKET ClientSocket;
private:

	void close()
	{
		int Ret = closesocket(ClientSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	SOCKET createClientSocket(const char* ADDR, int port)
	{
		const int ClientPort = port;
		SOCKET Client = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (Client == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}
		struct sockaddr_in6 ClientAddr;
		ZeroMemory(&ClientAddr, sizeof(ClientAddr));
		ClientAddr.sin6_family = AF_INET6;
		ClientAddr.sin6_addr = in6addr_any;
		ClientAddr.sin6_port = htons(ClientPort);
		int BindRes = ::bind(Client, (struct sockaddr *)&ClientAddr, sizeof(ClientAddr));
		if (BindRes == SOCKET_ERROR)
		{
			cerr << "bind() failed\n";
			WSACleanup();
			exit(1);
		}

		ipv6_mreq JoinAddr;
		inet_pton(AF_INET6, ADDR, &JoinAddr.ipv6mr_multiaddr);
		JoinAddr.ipv6mr_interface = 0;
		int Ret = setsockopt(Client, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, reinterpret_cast<char*>(&JoinAddr), sizeof(JoinAddr));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Client);
			Client = INVALID_SOCKET;
		}

		DWORD Timeout = 2000;
		Ret = setsockopt(Client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&Timeout), sizeof(Timeout));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			closesocket(Client);
			Client = INVALID_SOCKET;
		}

		return Client;
	}
};

string Username;

void sendMultiCast(SOCKET ServerSocket, bool IPV6, struct sockaddr_in*ADDR_V4, struct sockaddr_in6*ADDR_V6)
{
	while (true)
	{
		string Message = "Nut-mDiscovery v0.0.1 " + Username;
		int Ret = 0;
		if (!IPV6)
			Ret = sendto(ServerSocket, Message.c_str(), static_cast<int>(Message.size()), 0, reinterpret_cast<sockaddr*>(ADDR_V4), sizeof(*ADDR_V4));
		else
			Ret = sendto(ServerSocket, Message.c_str(), static_cast<int>(Message.size()), 0, reinterpret_cast<sockaddr*>(ADDR_V6), sizeof(*ADDR_V6));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "sendto() failed\n";
		}
		Sleep(200);
	}
}

void receiveMultiCast(SOCKET ClientSocket, bool IPV6)
{
	struct sockaddr_in ADDR_V4;
	struct sockaddr_in6 ADDR_V6;
	constexpr size_t CHUNK = 512;
	char Message[CHUNK];
	int Ret = 0;
	int fromlen;
	if (!IPV6)
	{
		fromlen = sizeof(ADDR_V4);
		Ret = recvfrom(ClientSocket, Message, CHUNK - 1, 0, reinterpret_cast<sockaddr*>(&ADDR_V4), &fromlen);
	}
	else
	{
		fromlen = sizeof(ADDR_V6);
		Ret = recvfrom(ClientSocket, Message, CHUNK - 1, 0, reinterpret_cast<sockaddr*>(&ADDR_V6), &fromlen);
	}
	if (Ret == SOCKET_ERROR)
	{
		cerr << "Error Code: " << GetLastError() << "\n";
		if (!IPV6)
			cerr << "Cannot get IPv4 address\n";
		else
			cerr << "Cannot get IPv6 address\n";
	}
	else
	{
		Message[Ret] = 0;
		cerr << "Message: " << Message << "\n";
		if (!IPV6)
		{
			char ADDRBuffer[INET_ADDRSTRLEN];
			const char *ADDR = inet_ntop(AF_INET, &ADDR_V4.sin_addr, ADDRBuffer,
				sizeof(ADDRBuffer));
			if (ADDR)
				cerr << "Server: " << ADDR << "\n";
			else
				cerr << "inet_ntop() err.\n";
		}
		else
		{
			char ADDRBuffer[INET6_ADDRSTRLEN];
			const char *ADDR = inet_ntop(AF_INET6, &ADDR_V6.sin6_addr, ADDRBuffer,
				sizeof(ADDRBuffer));
			if (ADDR)
				cerr << "Server: " << ADDR << "\n";
			else
				cerr << "inet_ntop() err.\n";
		}
	}
}

int main(int argc, char** argv)
{
	// Init Winsock2
	initWSA();
	// Declare the supported options.
	po::options_description desc("Usage");
	desc.add_options()
		("help", "Show help message")
		("server", "Work in server mode")
		("client", "Work in client mode")
		("port", po::value<int>(), "TCP port")
		("name", po::value<string>(), "username")
		;
	po::positional_options_description p;
	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
		po::notify(vm);
	}
	catch (exception&e)
	{
		cerr << e.what() << endl;
		return 1;
	}

	// help
	if (vm.count("help")) {
		cerr << desc << endl;
		WSACleanup();
		return 1;
	}

	if (vm.count("name"))
		Username = vm["name"].as<string>();

	if (vm.count("server") || vm.count("client"))
	{
		if (vm.count("server") && vm.count("client"))
		{
			cerr << "Cannot work under both server and client mode\n";
			WSACleanup();
			exit(1);
		}
		if (vm.count("server"))
		{
			if (!vm.count("port"))
			{
				cerr << "Please specify the TCP port\n";
				WSACleanup();
				return 1;
			}
			vector<thread>t;
			t.emplace_back([&]() {
				Server_v4 mDiscovery_Server_v4("224.0.0.1", vm["port"].as<int>());
				mDiscovery_Server_v4.serveForever();
				sendMultiCast(mDiscovery_Server_v4.ServerSocket, false, &mDiscovery_Server_v4.ServerAddr, nullptr);
			});
			t.emplace_back([&]() {
				Server_v6 mDiscovery_Server_v6("ff02::1", vm["port"].as<int>());
				mDiscovery_Server_v6.serveForever();
				sendMultiCast(mDiscovery_Server_v6.ServerSocket, true, nullptr, &mDiscovery_Server_v6.ServerAddr);
			});
			for (auto& i : t)
				i.join();
		}
		else
		{
			if (!vm.count("port"))
			{
				cerr << "Please specify the TCP port\n";
				WSACleanup();
				return 1;
			}
			if (vm.count("name"))
			{
				cerr << "name is ignored\n";
			}
			Client_v4 cl_v4("224.0.0.1", vm["port"].as<int>());
			Client_v6 cl_v6("ff02::1", vm["port"].as<int>());
			receiveMultiCast(cl_v4.ClientSocket, false);
			receiveMultiCast(cl_v6.ClientSocket, true);
		}
	}
	else
	{
		cerr << "Please specify the working mode (server/client)\n";
	}
	WSACleanup();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
