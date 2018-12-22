// Network.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <cstdlib>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <fstream>
#include <cstring>
#include <string>
#include <functional>
#include <sodium.h>
#include <thread>
#include <exception>
#include <boost/program_options.hpp>
#include <filesystem>
#include <Windows.h>
#include <zstd.h>
#include <malloc.h>

namespace fs = std::filesystem;

#pragma comment(lib, "ws2_32.lib")

namespace po = boost::program_options;

using namespace std;

constexpr size_t CHUNK_SIZE = 4096;

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
	Server_v4(const int port) :ServerPort(port), ServerSocket(INVALID_SOCKET) { crypto_kx_keypair(server_pk, server_sk); }

	void serveForever() {
		ServerSocket = createServerSocket(ServerPort);
		handleServerSocket(ServerSocket);
	}

	void setCallBack(function<void(const SOCKET, const char*, unsigned char*, unsigned char*)>CallBack)
	{
		Func = CallBack;
	}

	void close()
	{
		int Ret = closesocket(ServerSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}


private:
	SOCKET ServerSocket;

	int ServerPort;

	function<void(const SOCKET, const char*, unsigned char*, unsigned char*)> Func;

	SOCKET createServerSocket(int port)
	{
		const int ServerPort = port;
		SOCKET Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (Server == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}

		struct sockaddr_in ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin_family = AF_INET;
		ServerAddr.sin_addr = in4addr_any;
		ServerAddr.sin_port = htons(ServerPort);
		int BindRes = ::bind(Server, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
		if (BindRes == SOCKET_ERROR)
		{
			cerr << "bind() failed\n";
			WSACleanup();
			exit(1);
		}
		int Enable = 1;
		int Ret = setsockopt(Server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&Enable), sizeof(int));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			WSACleanup();
			exit(1);
		}
		int ListenRes = listen(Server, 128);
		if (ListenRes == SOCKET_ERROR)
		{
			cerr << "listen() failed\n";
			WSACleanup();
			exit(1);
		}
		return Server;
	}

	void handleServerSocket(SOCKET Server)
	{
		while (true)
		{
			SOCKET Client;
			struct sockaddr_in ClientAddr;
			int ClientAddrSize = sizeof(ClientAddr);
			Client = accept(Server, (struct sockaddr *)&ClientAddr, &ClientAddrSize);
			if (Client == INVALID_SOCKET)
			{
				cerr << "accept() failed\n";
				continue;
			}
			char ADDRBuffer[INET_ADDRSTRLEN];
			const char *ADDR = inet_ntop(AF_INET, &ClientAddr.sin_addr, ADDRBuffer,
				sizeof(ADDRBuffer));
			if (ADDR)
				cerr << "Connection Request: " << ADDR << ":" << ntohs(ClientAddr.sin_port) << "\n";
			else
				cerr << "inet_ntop() err.\n";
			std::thread(&Server_v4::handleClientRequest, this, Client, ADDR).detach();
		}
	}

	void handleClientRequest(const SOCKET Client, const char* ADDR)
	{
		Func(Client, ADDR, server_pk, server_sk);
		int Ret = closesocket(Client);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	// These two keys are reused each session.
	unsigned char server_pk[crypto_kx_PUBLICKEYBYTES], server_sk[crypto_kx_SECRETKEYBYTES];
};

class Server_v6
{
public:
	Server_v6(const int port) :ServerPort(port), ServerSocket(INVALID_SOCKET) { crypto_kx_keypair(server_pk, server_sk); }

	void serveForever() {
		ServerSocket = createServerSocket(ServerPort);
		handleServerSocket(ServerSocket);
	}

	void setCallBack(function<void(const SOCKET, const char*, unsigned char*, unsigned char*)>CallBack)
	{
		Func = CallBack;
	}

	void close()
	{
		int Ret = closesocket(ServerSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}


private:
	SOCKET ServerSocket;

	int ServerPort;

	function<void(const SOCKET, const char*, unsigned char*, unsigned char*)> Func;

	SOCKET createServerSocket(int port)
	{
		const int ServerPort = port;
		SOCKET Server = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (Server == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}

		struct sockaddr_in6 ServerAddr;
		ZeroMemory(&ServerAddr, sizeof(ServerAddr));
		ServerAddr.sin6_family = AF_INET6;
		ServerAddr.sin6_addr = in6addr_any;
		ServerAddr.sin6_port = htons(ServerPort);
		int BindRes = ::bind(Server, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
		if (BindRes == SOCKET_ERROR)
		{
			cerr << "bind() failed\n";
			WSACleanup();
			exit(1);
		}
		int Enable = 1;
		int Ret = setsockopt(Server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&Enable), sizeof(int));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "setsockopt() failed\n";
			WSACleanup();
			exit(1);
		}
		int ListenRes = listen(Server, 128);
		if (ListenRes == SOCKET_ERROR)
		{
			cerr << "listen() failed\n";
			WSACleanup();
			exit(1);
		}
		return Server;
	}

	void handleServerSocket(SOCKET Server)
	{
		while (true)
		{
			SOCKET Client;
			struct sockaddr_in6 ClientAddr;
			int ClientAddrSize = sizeof(ClientAddr);
			Client = accept(Server, (struct sockaddr *)&ClientAddr, &ClientAddrSize);
			if (Client == INVALID_SOCKET)
			{
				cerr << "accept() failed\n";
				continue;
			}
			char ADDRBuffer[INET6_ADDRSTRLEN];
			const char *ADDR = inet_ntop(AF_INET6, &ClientAddr.sin6_addr, ADDRBuffer,
				sizeof(ADDRBuffer));
			if (ADDR)
				cerr << "Connection Request: " << ADDR << ":" << ntohs(ClientAddr.sin6_port) << "\n";
			else
				cerr << "inet_ntop() err.\n";
			std::thread(&Server_v6::handleClientRequest, this, Client, ADDR).detach();
		}
	}

	void handleClientRequest(const SOCKET Client, const char* ADDR)
	{
		Func(Client, ADDR, server_pk, server_sk);
		int Ret = closesocket(Client);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}

	// These two keys are reused each session.
	unsigned char server_pk[crypto_kx_PUBLICKEYBYTES], server_sk[crypto_kx_SECRETKEYBYTES];
};

class Client_v4
{
public:
	Client_v4(const char* ADDR, int port)
	{
		ClientSocket = createClientSocket(ADDR, port);
	}
	void close()
	{
		int Ret = closesocket(ClientSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}
	SOCKET ClientSocket;
private:
	SOCKET createClientSocket(const char* ADDR, int port)
	{
		const int ClientPort = port;
		SOCKET Client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (Client == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}
		struct sockaddr_in ClientAddr;
		ZeroMemory(&ClientAddr, sizeof(ClientAddr));
		ClientAddr.sin_family = AF_INET;
		inet_pton(AF_INET, ADDR, &ClientAddr.sin_addr);
		ClientAddr.sin_port = htons(ClientPort);
		int Ret = connect(Client, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "connect() failed\n";
			int Ret = closesocket(Client);
			if (Ret == SOCKET_ERROR)
			{
				cerr << "close() failed\n";
			}
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
	void close()
	{
		int Ret = closesocket(ClientSocket);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "close() failed\n";
		}
	}
	SOCKET ClientSocket;
private:
	SOCKET createClientSocket(const char* ADDR, int port)
	{
		const int ClientPort = port;
		SOCKET Client = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (Client == INVALID_SOCKET)
		{
			cerr << "socket() failed\n";
			WSACleanup();
			exit(1);
		}
		struct sockaddr_in6 ClientAddr;
		ZeroMemory(&ClientAddr, sizeof(ClientAddr));
		ClientAddr.sin6_family = AF_INET6;
		inet_pton(AF_INET6, ADDR, &ClientAddr.sin6_addr);
		ClientAddr.sin6_port = htons(ClientPort);
		int Ret = connect(Client, (struct sockaddr*)&ClientAddr, sizeof(ClientAddr));
		if (Ret == SOCKET_ERROR)
		{
			cerr << "connect() failed\n";
			int Ret = closesocket(Client);
			if (Ret == SOCKET_ERROR)
			{
				cerr << "close() failed\n";
			}
			Client = INVALID_SOCKET;
		}
		return Client;
	}
};

string FileName;
void handleClient(const SOCKET ServerSocket, const char* ADDR, unsigned char* server_pk, unsigned char* server_sk)
{
	// Key Exchange
	//=======================================================
	unsigned char client_pk[crypto_kx_PUBLICKEYBYTES];
	unsigned char server_rx[crypto_kx_SESSIONKEYBYTES], server_tx[crypto_kx_SESSIONKEYBYTES];
	// Client send public key first
	int Ret = recv(ServerSocket, reinterpret_cast<char*>(client_pk), crypto_kx_PUBLICKEYBYTES, MSG_WAITALL);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "recv() failed\n";
		return;
	}
	Ret = send(ServerSocket, reinterpret_cast<char*>(server_pk), crypto_kx_PUBLICKEYBYTES, 0);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "send() failed\n";
		return;
	}
	if (crypto_kx_server_session_keys(server_rx, server_tx,
		server_pk, server_sk, client_pk) != 0) {
		cerr << "Suspicious client public key\n";
		return;
	}
	cerr << "Server Key Exchange done\n";
	//=======================================================
	// Server only receives, so we only use server_rx here.
	unsigned char  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
	unsigned char  buf_out[CHUNK_SIZE];
	unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	crypto_secretstream_xchacha20poly1305_state st;
	// Read header
	int gcount = recv(ServerSocket, reinterpret_cast<char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES, MSG_WAITALL);
	if (gcount != crypto_secretstream_xchacha20poly1305_HEADERBYTES)
	{
		cerr << "The input stream is broken!\n";
		return;
	}
	if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, server_rx) != 0) {
		// incomplete header
		cerr << "The encryption header is incomplete\n";
		return;
	}

	unsigned char tag = 0;
	unsigned long long out_len;
	int rlen;

	// Read filename
	rlen = recv(ServerSocket, reinterpret_cast<char*>(buf_in), CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES, MSG_WAITALL);
	if (rlen == SOCKET_ERROR)
	{
		cerr << "recv() failed\n";
		return;
	}

	if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
		buf_in, rlen, NULL, 0) != 0) {
		// corrupted chunk
		cerr << "The file_name is corrupted\n";
		return;
	}

	if (tag != crypto_secretstream_xchacha20poly1305_TAG_PUSH) {
		cerr << "The file_name_tag is corrupted\n";
		return;
	}


	char fname[_MAX_FNAME];
	char fext[_MAX_EXT];
	_splitpath_s(reinterpret_cast<const char*>(buf_out),
		nullptr, 0, nullptr, 0, fname, _MAX_FNAME, fext, _MAX_EXT);
	string Output = ".\\Files\\" + string(fname) + string(fext);
	cerr << "Receiving " << Output << endl;
	ofstream os(Output, ios::binary | ios::trunc | ios::out);

	// Decompress

	// Create pipe for decompressing.
	HANDLE hReadPipe, hWritePipe;
	Ret = CreatePipe(&hReadPipe, &hWritePipe, NULL, static_cast<DWORD>(max(ZSTD_DStreamInSize(), CHUNK_SIZE)));
	if (Ret == 0)
	{
		cerr << "CreatePipe() failed\n";
		os.close();
		remove(Output.c_str());
		return;
	}

	auto Decompressor = [&os, &hReadPipe, &Output]()
	{
		size_t const buffInSize = ZSTD_DStreamInSize();
		// Guarantee to successfully flush at least one complete compressed block in all circumstances.
		size_t const buffOutSize = ZSTD_DStreamOutSize();
		void* const buffIn = malloc(buffInSize);
		void* const buffOut = malloc(buffOutSize);
		ZSTD_DStream* const dstream = ZSTD_createDStream();
		if (dstream == nullptr)
		{
			cerr << "ZSTD_createDStream() failed\n";
			os.close();
			remove(Output.c_str());
			// Inside thread, we use abort().
			abort();
		}
		size_t const initResult = ZSTD_initDStream(dstream);
		if (ZSTD_isError(initResult))
		{
			cerr << "ZSTD_initDStream() error : " << ZSTD_getErrorName(initResult) << "\n";
			os.close();
			remove(Output.c_str());
			// Inside thread, we use abort().
			abort();
		}

		// Decompressing...
		size_t toRead = initResult;
		// Ensure the sizeof char is 1
		static_assert(sizeof(char) == 1);

		bool eof = false;
		do
		{
			DWORD rlen, real_rlen = 0;
			DWORD TEMP_CHUNK_SIZE = static_cast<DWORD>(toRead);
			char* temp_buf_in = reinterpret_cast<char*>(buffIn);
			BOOL Ret = ReadFile(hReadPipe, temp_buf_in, TEMP_CHUNK_SIZE, &rlen, NULL);
			real_rlen += rlen; // Add rlen to rrlen
			// EOF is not reached.
			while (Ret == TRUE && rlen != TEMP_CHUNK_SIZE)
			{
				TEMP_CHUNK_SIZE -= rlen;
				temp_buf_in += rlen;
				Ret = ReadFile(hReadPipe, temp_buf_in, TEMP_CHUNK_SIZE, &rlen, NULL);
				real_rlen += rlen; // Add rlen to rrlen
			}
			// EOF is reached
			eof = ((Ret == FALSE) && (GetLastError() == ERROR_BROKEN_PIPE));

			ZSTD_inBuffer input = { buffIn, real_rlen, 0 };
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };

			while (input.pos < input.size || output.pos == output.size)
			{
				output = { buffOut, buffOutSize, 0 };
				toRead = ZSTD_decompressStream(dstream, &output, &input);
				if (ZSTD_isError(toRead))
				{
					cerr << "ZSTD_decompressStream() error : " << ZSTD_getErrorName(toRead) << "\n";
					os.close();
					remove(Output.c_str());
					break;
				}
				os.write(reinterpret_cast<char*>(buffOut), output.pos);
			}
		} while (!eof);

		// Cleanup Resources
		ZSTD_freeDStream(dstream);
		::free(buffIn);
		::free(buffOut);
		os.close();
		{
			int Ret = CloseHandle(hReadPipe);
			if (Ret == 0)
			{
				cerr << "Decompressor CloseHandle() filed" << endl;
			}
		}
	};

	thread t(Decompressor);

	while (true)
	{
		rlen = recv(ServerSocket, reinterpret_cast<char*>(buf_in), CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES, MSG_WAITALL);
		if (rlen == SOCKET_ERROR)
		{
			cerr << "recv() failed\n";
			// Force close, even for the decompressor.
			os.close();
			remove(Output.c_str());
			break;
		}
		if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
			buf_in, rlen, NULL, 0) != 0) {
			// corrupted chunk
			cerr << "The file is corrupted\n";
			// Force close, even for the decompressor.
			os.close();
			remove(Output.c_str());
			break;
		}
		DWORD NumberOfBytesWritten;
		{
			BOOL Ret = WriteFile(hWritePipe, buf_out, static_cast<DWORD>(out_len), &NumberOfBytesWritten, NULL);
			if (Ret == FALSE)
			{
				cerr << "WriteFile() failed\n";
				// Force close, even for the decompressor.
				os.close();
				remove(Output.c_str());
				break;
			}
		}
		if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL)
			break;
	}


	// Finish Compressor execution
	Ret = CloseHandle(hWritePipe);
	if (Ret == 0)
	{
		cerr << "CloseHandle() filed" << endl;
	}

	t.join();

	Ret = shutdown(ServerSocket, SD_BOTH);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "shutdown() failed\n";
	}
	cerr << "The file has been successfully received\n";
}


void handleServer(const SOCKET ClientSocket)
{
	if (ClientSocket == INVALID_SOCKET)
		return;
	// Key Exchange
	//=======================================================
	unsigned char server_pk[crypto_kx_PUBLICKEYBYTES];
	unsigned char client_pk[crypto_kx_PUBLICKEYBYTES], client_sk[crypto_kx_SECRETKEYBYTES];
	unsigned char client_rx[crypto_kx_SESSIONKEYBYTES], client_tx[crypto_kx_SESSIONKEYBYTES];
	crypto_kx_keypair(client_pk, client_sk);
	// Client send public key first
	int Ret = send(ClientSocket, reinterpret_cast<char*>(client_pk), crypto_kx_PUBLICKEYBYTES, 0);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "send() failed\n";
		return;
	}
	// Get Server's public key
	int ReadLen = recv(ClientSocket, reinterpret_cast<char*>(server_pk), crypto_kx_PUBLICKEYBYTES, MSG_WAITALL);
	if (ReadLen == SOCKET_ERROR)
	{
		cerr << "recv() failed\n";
		return;
	}
	if (crypto_kx_client_session_keys(client_rx, client_tx,
		client_pk, client_sk, server_pk) != 0) {
		cerr << "Suspicious server public key\n";
		return;
	}
	cerr << "Client Key Exchange done\n";
	//=======================================================
	// Client only sends, so we only use client_tx here.
	ifstream is(FileName, ios::binary | ios::in);
	if (!is.is_open())
	{
		cerr << "The input file cannot be opened!\n";
		return;
	}
	unsigned char  buf_in[CHUNK_SIZE];
	unsigned char  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
	unsigned char  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	crypto_secretstream_xchacha20poly1305_state st;
	crypto_secretstream_xchacha20poly1305_init_push(&st, header, client_tx);
	// Send header
	Ret = send(ClientSocket, reinterpret_cast<const char*>(header), crypto_secretstream_xchacha20poly1305_HEADERBYTES, 0);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "send() failed\n";
		is.close();
		return;
	}
	bool eof = is.eof();
	unsigned char tag = 0;
	unsigned long long out_len;
	DWORD rlen;
	// Send filename
	// Because filename is trivially small, so we do not compress it and send it as a whole.
	memset(buf_in, 0, sizeof(buf_in));
	strcpy_s(reinterpret_cast<char*>(buf_in), CHUNK_SIZE, FileName.c_str());
	crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len,
		buf_in, CHUNK_SIZE,
		NULL, 0, crypto_secretstream_xchacha20poly1305_TAG_PUSH);
	Ret = send(ClientSocket, reinterpret_cast<char*>(buf_out), static_cast<int>(out_len), 0);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "send() failed\n";
		is.close();
		return;
	}

	// Send file

	// Create pipe for compressing.
	HANDLE hReadPipe, hWritePipe;
	Ret = CreatePipe(&hReadPipe, &hWritePipe, NULL, static_cast<DWORD>(max(ZSTD_CStreamOutSize(), CHUNK_SIZE)));
	if (Ret == 0)
	{
		cerr << "CreatePipe() failed\n";
		is.close();
		return;
	}

	auto Compressor = [&is, &hWritePipe]()
	{
		// Allocate Resources
		bool eof;
		size_t const buffInSize = ZSTD_CStreamInSize();
		size_t const buffOutSize = ZSTD_CStreamOutSize();
		void* const buffIn = malloc(buffInSize);
		void* const buffOut = malloc(buffOutSize);
		ZSTD_CStream* const cstream = ZSTD_createCStream();
		if (cstream == nullptr)
		{
			cerr << "ZSTD_createCStream() failed\n";
			is.close();
			// Inside thread, we use abort().
			abort();
		}
		size_t const initResult = ZSTD_initCStream(cstream, ZSTD_CLEVEL_DEFAULT);
		if (ZSTD_isError(initResult))
		{
			cerr << "ZSTD_initCStream() error : " << ZSTD_getErrorName(initResult) << "\n";
			// Inside thread, we use abort().
			abort();
		}

		// Compressing...
		size_t rlen, toRead = buffInSize;
		// Ensure the sizeof char is 1
		static_assert(sizeof(char) == 1);
		do
		{
			is.read(reinterpret_cast<char*>(buffIn), toRead);
			rlen = is.gcount();
			eof = is.eof();
			ZSTD_inBuffer input = { buffIn,rlen,0 };
			while (input.pos < input.size)
			{
				ZSTD_outBuffer output = { buffOut, buffOutSize,0 };
				// toRead is guaranteed to be <= ZSTD_CStreamInSize()
				toRead = ZSTD_compressStream(cstream, &output, &input);
				if (ZSTD_isError(toRead))
				{
					cerr << "ZSTD_compressStream() error : " << ZSTD_getErrorName(initResult) << "\n";
					// Inside thread, we use abort().
					abort();
				}
				// Safe to modify buffInSize.
				if (toRead > buffInSize)
					toRead = buffInSize;
				// Write to pipe.
				DWORD NumberOfBytesWritten;
				BOOL Ret = WriteFile(hWritePipe, buffOut, static_cast<DWORD>(output.pos), &NumberOfBytesWritten, NULL);
				if (Ret == FALSE)
				{
					cerr << "WriteFile1() error: " << GetLastError() << "\n";
					// Inside thread, we use abort().
					abort();
				}
			}
		} while (!eof);
		ZSTD_outBuffer output = { buffOut, buffOutSize,0 };
		size_t const remainingToFlush = ZSTD_endStream(cstream, &output);   /* close frame */
		if (remainingToFlush)
		{
			cerr << "not fully flushed" << "\n";
			// Inside thread, we use abort().
			abort();
		}
		// Write to pipe.
		DWORD NumberOfBytesWritten;
		BOOL Ret = WriteFile(hWritePipe, buffOut, static_cast<DWORD>(output.pos), &NumberOfBytesWritten, NULL);
		if (Ret == FALSE)
		{
			cerr << "WriteFile2() error: " << GetLastError() << "\n";
			// Inside thread, we use abort().
			abort();
		}

		// Cleanup Resources
		ZSTD_freeCStream(cstream);
		::free(buffIn);
		::free(buffOut);
		{
			int Ret = CloseHandle(hWritePipe);
			if (Ret == 0)
			{
				cerr << "Compressor CloseHandle() filed" << endl;
			}
		}
	};

	thread t(Compressor);

	do
	{
		DWORD real_rlen = 0;
		DWORD TEMP_CHUNK_SIZE = static_cast<DWORD>(CHUNK_SIZE);
		unsigned char* temp_buf_in = buf_in;
		BOOL Ret = ReadFile(hReadPipe, temp_buf_in, TEMP_CHUNK_SIZE, &rlen, NULL);
		real_rlen += rlen; // Add rlen to rrlen

		// EOF is not reached.
		while (Ret == TRUE && rlen != TEMP_CHUNK_SIZE)
		{
			TEMP_CHUNK_SIZE -= rlen;
			temp_buf_in += rlen;
			Ret = ReadFile(hReadPipe, temp_buf_in, TEMP_CHUNK_SIZE, &rlen, NULL);
			real_rlen += rlen; // Add rlen to rrlen
		}
		// EOF is reached
		eof = ((Ret == FALSE) && (GetLastError() == ERROR_BROKEN_PIPE));

		tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
		crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, real_rlen,
			NULL, 0, tag);
		Ret = send(ClientSocket, reinterpret_cast<char*>(buf_out), static_cast<int>(out_len), 0);
		if (Ret == SOCKET_ERROR)
		{
			cerr << "send() failed\n";
			break;
		}
	} while (!eof);

	// Finish Compressor execution
	Ret = CloseHandle(hReadPipe);
	if (Ret == 0)
	{
		cerr << "CloseHandle() filed" << endl;
	}

	t.join();

	Ret = shutdown(ClientSocket, SD_BOTH);
	if (Ret == SOCKET_ERROR)
	{
		cerr << "shutdown() failed\n";
	}
	is.close();
}


int main(int argc, char** argv)
{
	// Init crypto library
	if (sodium_init() == -1) {
		std::cerr << "libsodium failed to init, exiting...\n";
		return 1;
	}
	// Create Files
	fs::create_directory(".\\Files");
	// Init Winsock2
	initWSA();
	// Declare the supported options.
	po::options_description desc("Usage");
	desc.add_options()
		("help", "Show help message")
		("server", "Work in server mode")
		("client", "Work in client mode")
		("port", po::value<int>(), "TCP port")
		("ipv4", po::value<string>(), "IPv4 address of the server")
		("ipv6", po::value<string>(), "IPv6 address of the server")
		("file", po::value<string>(), "File to be sent")
		;
	po::positional_options_description p;
	p.add("file", -1);
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
			if (vm.count("ipv4") || vm.count("ipv6"))
			{
				cerr << "Address is ignored\n";
			}
			if (vm.count("file"))
			{
				cerr << "File is ignored\n";
			}
			vector<thread>t;
			t.emplace_back([&]() {
				Server_v4 FileTransferServer_v4(vm["port"].as<int>());
				FileTransferServer_v4.setCallBack(handleClient);
				FileTransferServer_v4.serveForever();
				FileTransferServer_v4.close();
			});
			t.emplace_back([&]() {
				Server_v6 FileTransferServer_v6(vm["port"].as<int>());
				FileTransferServer_v6.setCallBack(handleClient);
				FileTransferServer_v6.serveForever();
				FileTransferServer_v6.close();
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
			if (!vm.count("ipv4") && !vm.count("ipv6"))
			{
				cerr << "Please specify the IP address\n";
				WSACleanup();
				return 1;
			}
			if (vm.count("ipv4") && vm.count("ipv6"))
			{
				cerr << "Please specify only one IP address\n";
				WSACleanup();
				return 1;
			}
			if (!vm.count("file"))
			{
				cerr << "Please specify the File\n";
				WSACleanup();
				return 1;
			}
			if (vm.count("ipv4"))
			{
				Client_v4 cl(vm["ipv4"].as<string>().c_str(), vm["port"].as<int>());
				FileName = vm["file"].as<string>();
				handleServer(cl.ClientSocket);
				cl.close();
			}
			else
			{
				Client_v6 cl(vm["ipv6"].as<string>().c_str(), vm["port"].as<int>());
				FileName = vm["file"].as<string>();
				handleServer(cl.ClientSocket);
				cl.close();
			}

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
