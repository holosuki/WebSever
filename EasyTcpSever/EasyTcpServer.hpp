#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN //记得写这句话，不然下面两个include有问题；
	#include <windows.h>
	#include <winsock2.h>
	//#pragma comment(lib,"ws2_32.lib") //仅在windows里适用，通用的实在属性里加ws2_32.lib；
#else		//linux或者MacOS
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <iostream>
#include <stdio.h>
#include <thread>
#include <vector>

#include "MessageHeader.hpp"

class EasyTcpServer {
private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化SOCKET
	void initSocket() {
#ifdef _WIN32
		//启动Windows socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			printf("close old socket\n");
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			printf("build socket failed\n");
		}
		else
		{
			printf("build socket successed\n");
		}
	}

	//绑定ip端口号
	void Bind(const char* ip, short port) {

		if (INVALID_SOCKET == _sock)
		{
			initSocket();
		}
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip) _sin.sin_addr.S_un.S_addr = inet_addr(ip);////不限定访问Ip地址
		else _sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");////不限定访问Ip地址
#else 
		if (ip) _sin.sin_addr.s_addr = inet_addr(ip);////不限定访问Ip地址
		else _sin.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");////不限定访问Ip地址
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (ret == SOCKET_ERROR) printf("Bind Port failed,Port<%d>\n", port);
		else printf("Bind Port Sucessed\n");
	}

	//监听端口号
	void Listen(int n) {
		//listen监听网络端口
		int ret = listen(_sock, n);
		if (ret == SOCKET_ERROR) printf("<socket=%d>Listen Port failed\n", _sock);
		else printf("<socket=%d>Listen Port Sucessed\n", (int)_sock);
	}

	//接收客户端信息
	void AcceptClient(){
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET _csock = INVALID_SOCKET;

#ifdef _WIN32
		_csock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else 
		_csock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (_csock == INVALID_SOCKET) printf("failed，Invalid client received\n");
		else {
			NewUser userJoin;
			Send2ALLData(&userJoin);
			printf("New Client:SOCKET = %d  IP = %s\n", (int)_csock, inet_ntoa(clientAddr.sin_addr));
			g_clients.push_back(_csock);
		}

	}
	bool isRun() { return _sock != INVALID_SOCKET;}

	bool OnRun() {
		if (isRun()) {
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);
			for (int i = g_clients.size() - 1; i >= 0; i--) {
				FD_SET(g_clients[i], &fdRead);
			}
			timeval time = { 1,0 };
			int ret = select(_sock + 1, &fdRead, 0, 0, &time);
			if (ret < 0) {
				printf("failed，closed\n");
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				//accept等待客户端链接
				AcceptClient();
			}
#ifdef _WIN32
			for (size_t k = 0; k < fdRead.fd_count; ++k)
			{
				if (-1 == RecvData(fdRead.fd_array[k]))
				{
					auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[k]);
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}
			}
#else
			for (int i = (int)g_clients.size() - 1; i >= 0; --i)
			{
				if (FD_ISSET(g_clients[i], &fdRead)) //判断有没有
				{
					if (-1 == RecvData(g_clients[i]))
					{
						auto iter = g_clients.begin();
						if (iter != g_clients.end())
						{
							g_clients.erase(iter);
						}
					}
				}
			}
#endif
			//printf("other work doing\n");

			return true;
		}
		return false;

	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int k = 0; k < (int)g_clients.size(); ++k)
			{
				closesocket(g_clients[k]);
			}
			//6 关闭socket

			closesocket(_sock);
			//————
			//清除环境
			WSACleanup();
#else
			for (int k = 0; k < (int)g_clients.size(); ++k)
			{
				close(g_clients[k]);
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	int RecvData(SOCKET _csock) {
		char szRecv[1024] = {};
		//接收客户端请求
		int nlen = (int)recv(_csock, szRecv, sizeof(DataHeader), 0);
		DataHeader* _header = (DataHeader*)szRecv;
		if (nlen <= 0) {
			printf("client<Socket=%d> end\n", (int)_csock);
			return -1;
		}
		recv(_csock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(_csock, _header);
		return 0;
	}

	void OnNetMsg(SOCKET _csock, DataHeader* _header) {
		switch (_header->cmd) {
		case CMD_LOGIN:
		{
			Login* login = (Login*)_header;
			printf("Received CLient<Socket=%d>command:  Data Length= %d  user = %s pass = %s\n", (int)_csock, login->dataLength, login->userName, login->passWord);
			LoginResult res;
			send(_csock, (char*)&res, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			LogOut* logout = (LogOut*)_header;
			printf("Received command: user=%s\n", logout->userName);
			LogoutResult _lgres;
			send(_csock, (char*)&_lgres, sizeof(LogoutResult), 0);
		}
		break;
		default:
			DataHeader hd = { 0,CMD_ERROR };
			send(_csock, (const char*)&hd, sizeof(hd), 0);
			break;
		}
	}

	//发送指定SOCKET数据
	int SendData(SOCKET _csock, DataHeader* _header) {
		if (isRun() && _header) {
			return send(_csock, (const char*)_header, _header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void Send2ALLData(DataHeader* _header) {
		if (isRun() && _header) {
			for (int n = g_clients.size() - 1; n >= 0; n--) {
				SendData(g_clients[n], _header);
			}
		}
	}
};

#endif