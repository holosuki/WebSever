#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN //记得写这句话，不然下面两个include有问题；
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <vector>

//#pragma comment(lib,"ws2_32.lib") //仅在windows里适用，通用的实在属性里加ws2_32.lib；

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

struct DataHeaader
{
	short dataLength;
	short cmd;
};

struct Login :public DataHeaader
{
	Login()
	{
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];

};

struct LoginResult :public DataHeaader
{
	LoginResult()
	{
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}
	int result;
};

struct LogOut :public DataHeaader
{
	LogOut()
	{
		dataLength = sizeof(LogOut);
		cmd = CMD_LOGOUT;

	}
	char userName[32];
};

struct LogoutResult :public DataHeaader
{
	LogoutResult()
	{
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;
};

struct NewUser :public DataHeaader
{
	NewUser()
	{
		dataLength = sizeof(NewUser);
		cmd = CMD_NEW_USER_JOIN;
		sock_id = 0;
	}
	int sock_id;
};

std::vector <SOCKET> g_client;

int ProcessorFunction(SOCKET _csock)
{
	char szRecv[1024] = {};
	//接收客户端请求
	int nlen = recv(_csock, szRecv, sizeof(DataHeaader), 0);
	DataHeaader* _header = (DataHeaader*)szRecv;
	if (nlen <= 0)
	{
		printf("client<Socket=%d> end\n", _csock);
		return -1;
	}
	switch (_header->cmd)
	{
		case CMD_LOGIN:
		{
			recv(_csock, szRecv + sizeof(DataHeaader), _header->dataLength - sizeof(DataHeaader), 0);
			Login* login = (Login*)szRecv;
			printf("Received CLient<Socket=%d>command:  Data Length= %d  user = %s pass = %s\n", _csock, login->dataLength, login->userName, login->passWord);
			LoginResult res;
			send(_csock, (char*)&res, sizeof(LoginResult), 0);

		}
		break;
		case CMD_LOGOUT:
		{
			recv(_csock, szRecv + sizeof(DataHeaader), _header->dataLength - sizeof(DataHeaader), 0);
			LogOut* logout = (LogOut*)szRecv;
			printf("Received command: user=%s\n", logout->userName);
			LogoutResult _lgres;
			send(_csock, (char*)&_lgres, sizeof(LogoutResult), 0);

		}
		break;
		default:
		{
			DataHeaader hd = { 0,CMD_ERROR };
			send(_csock, (const char*)&hd, sizeof(hd), 0);
		}
		break;
	}
	return 0;
}

int main()
{
	//启动windows socket 2.x 环境；下面是2.2；
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//————

	//--用socket API建立简易TCP服务端
	//1 建立一个SOCKET
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//AF_INET ipv4,SOCK_STREAM 数据流，TCP协议
	//2 bind 绑定用于接受客户端连接的网络端口
	sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(4567);//host to net unsigned short,主机地址转换成网络无符号短整型地址；
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;// inet_addr("127.0.0.1"),内网链接;或者 INADDR_ANY 任意本机地址；
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
	{
		printf("ERROR 绑定用于接受客户端连接的网络端口失败\n");
	}
	else
	{
		printf("绑定用于接受客户端连接的网络端口成功\n");
	}
	//3 listen 监听网络端口
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("ERROR 监听网络端口失败\n");
	}
	else
	{
		printf("监听网络端口成功\n");
	}
	char _recvbuf[128] = {};
	while (true)
	{
		//伯克利 BSD socket select参数
		fd_set fdRead;	//描述符集合
		fd_set fdWrite;
		fd_set fdExp;
		FD_ZERO(&fdRead);	//清空
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (int i = g_client.size() - 1; i >= 0; --i)
		{
			FD_SET(g_client[i], &fdRead);
		}

		timeval time = {0, 0};	//定义阻塞或者非阻塞， 1是定义的最大等待时间

		//nfds 是一个整数值，是指fd_set集合中的所有描述符范围，而不是数量，最大描述符+1
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &time);
		if (ret < 0)
		{
			printf("failed,close \n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) //判断有没有
		{
			FD_CLR(_sock, &fdRead);
			//4 accept 等待接收客户端连接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _clientSock = INVALID_SOCKET;
			_clientSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _clientSock)
			{
				printf("ERROR 接收到无效客户端\n");
			}
			for (int n = g_client.size() - 1; n >= 0; --n)
			{
				NewUser userJoin;
				send(g_client[n], (const char*)&userJoin, sizeof(NewUser), 0);
			}
			printf("新客户端加入：SOCKET = %d IP = %s \n", _clientSock,inet_ntoa(clientAddr.sin_addr));
			g_client.push_back(_clientSock);
		}
		for (size_t k = 0; k < fdRead.fd_count; ++k)
		{
			if(-1 == ProcessorFunction(fdRead.fd_array[k]))
			{
				auto iter = find(g_client.begin(), g_client.end(), fdRead.fd_array[k]);
				if (iter != g_client.end())
				{
					g_client.erase(iter);
				}
			}
		}
		//printf("空闲时间执行其他任务。\n");
	}
	for (int k = 0; k < (int)g_client.size(); ++k)
	{
		closesocket(g_client[k]);
	}
	//6 关闭socket
	closesocket(_sock);
	//————
	//清除环境
	WSACleanup();
	printf("服务器关闭\n");
	getchar();
	return 0;
}