#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <functional>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

class TCPClient {

public:
	std::string hostname = "";
	std::string port = "";
	int recieveBufferSize = 8000;
	std::string helloMessage = "";

	int sendMessage(std::string messageToSend) {
		 return send(this->curSocket, messageToSend.c_str(), strlen(messageToSend.c_str()), 0);
	}
	
	static void bandAid(void* thisptr) {
		//can't CreateThread with private functions :( i cri
		((TCPClient*)thisptr)->recieveMessageThread();
	}

	TCPClient(std::string Hostname, std::string Port, int BufLen, std::string HelloMessage, void (*messageRecieveEventPtr)(std::string message)) {
		this->hostname = Hostname;
		this->port = Port;
		this->recieveBufferSize = BufLen;
		this->helloMessage = HelloMessage;
		this->messageRecieveEvent = messageRecieveEventPtr;
		ZeroMemory(&this->hints, sizeof(this->hints));
		recieveBuffer.resize(recieveBufferSize);

		iResult = WSAStartup(MAKEWORD(2, 2), &this->wsaData);
		if (iResult != 0) {
			throw std::exception("WSAStartup failed with code " + WSAGetLastError());
		}
		this->hints.ai_family = AF_UNSPEC;
		this->hints.ai_socktype = SOCK_STREAM;
		this->hints.ai_protocol = IPPROTO_TCP;

		this->iResult = GetAddrInfo(this->hostname.c_str(), this->port.c_str(), &this->hints, &this->info);
		if (this->iResult != 0) {
			throw std::exception("GetAddrInfo failed with code " + WSAGetLastError());
		}

		this->curSocket = socket(this->info->ai_family, this->info->ai_socktype, this->info->ai_protocol);
		if (this->curSocket == INVALID_SOCKET) {
			throw std::exception("socket() failed with code " + WSAGetLastError());
		}

		this->iResult = connect(this->curSocket, this->info->ai_addr, this->info->ai_addrlen);
		if (this->iResult == SOCKET_ERROR) {
			throw std::exception("connect() failed with code " + WSAGetLastError());
		}

		if (sendMessage("helloMessage") == -1) {
			throw std::exception("send() failed with code " + WSAGetLastError());
		}
		this->recieveMessageThreadHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)& bandAid, this, 0, nullptr);
	}

	~TCPClient() {
		delete(this->info);
		closesocket(this->curSocket);
		WSACleanup();
		//lazy
		TerminateThread(recieveMessageThreadHandle, 0);
		CloseHandle(this->recieveMessageThreadHandle);
	}

private:
	void recieveMessageThread() {
		do {
			this->iResult = recv(this->curSocket, (char*)this->recieveBuffer.data(), recieveBuffer.size(), 0);
			this->messageRecieveEvent(recieveBuffer);
		} while (this->iResult > 0);
		if (!iResult)
			throw std::exception("Connection closed.");
		else throw std::exception("recv() failed with code " + WSAGetLastError());
	}

	WSADATA wsaData;
	int iResult = 0;
	addrinfo* info = nullptr, hints;
	SOCKET curSocket = INVALID_SOCKET;
	HANDLE recieveMessageThreadHandle = INVALID_HANDLE_VALUE;
	std::string recieveBuffer = "";
	void (*messageRecieveEvent)(std::string message);
};
