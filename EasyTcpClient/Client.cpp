#define WIN32_LEAN_AND_MEAN //记得写这句话，不然下面两个include有问题；
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <thread>

//#pragma comment(lib,"ws2_32.lib") //仅在windows里适用，通用的实在属性里加ws2_32.lib；

//命令列表
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

//数据包头
//前期测试用short
struct DataHeaader
{
	short dataLength;	//数据长度
	short cmd;			//命令
};

//数据包体
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

int ProcessorFunction(SOCKET _csock)
{
	char szRecv[1024] = {};
	//接收客户端请求
	int nlen = recv(_csock, szRecv, sizeof(DataHeaader), 0);
	DataHeaader* _header = (DataHeaader*)szRecv;
	if (nlen <= 0)
	{
		printf("disconnect...\n", _csock);
		return -1;
	}
	switch (_header->cmd)
	{
		case CMD_LOGIN_RESULT:
		{
			recv(_csock, szRecv + sizeof(DataHeaader), _header->dataLength - sizeof(DataHeaader), 0);
			LoginResult* Loginres = (LoginResult*)szRecv;
			printf("Received server data CMD_LOGIN_RESULT:  Data Length= %d \n", Loginres->dataLength);
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			recv(_csock, szRecv + sizeof(DataHeaader), _header->dataLength - sizeof(DataHeaader), 0);
			LogoutResult* Logoutres = (LogoutResult*)szRecv;
			printf("Received server data CMD_LOGOUT_RESULT:  Data Length= %d \n", Logoutres->dataLength);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			recv(_csock, szRecv + sizeof(DataHeaader), _header->dataLength - sizeof(DataHeaader), 0);
			NewUser* newUserJoin = (NewUser*)szRecv;
			printf("Received server data CMD_NEW_USER_JOIN:  Data Length= %d \n", newUserJoin->dataLength);
		}
		break;
	}
}

bool g_bRun = true;
//客户端输入
void cmdThread(SOCKET _sock) {
	while (true) {
		char cmdbuf[256] = {};
		scanf("%s", cmdbuf);
		if (0 == strcmp(cmdbuf, "exit")) {
			g_bRun = false;
			printf("close\n");
			return;
		}
		else if (0 == strcmp(cmdbuf, "login")) {
			Login log;
			strcpy(log.userName, "张国荣");
			strcpy(log.passWord, "23");
			send(_sock, (const char*)&log, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdbuf, "login")) {
			LogOut logout;
			strcpy(logout.userName, "张国荣");
			send(_sock, (const char*)&logout, sizeof(LogOut), 0);
		}
		else {
			printf("commond error!\n");
		}
	}
}

int main()
{
	//启动windows socket 2.x 环境；下面是2.2；
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//————

	//--用socket API建立简易TCP客户端
	//1 建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("建立socket失败\n");
	}
	else
	{
		printf("建立socket成功\n");
	}
	//2 连接服务器 connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");// INADDR_ANY;
	int ret = connect(_sock,(sockaddr*)&_sin,sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("连接服务器失败\n");
	}
	else
	{
		printf("连接服务器成功\n");
	}
	
	//线程
	std::thread t1(cmdThread, _sock);
	t1.detach();	//与主线程分离，因为主线程先退出

	while (g_bRun)
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);
		timeval time = { 1,0 };
		int ret = select(_sock, &fdRead, NULL, NULL, &time);
		if (ret < 0)
		{
			printf("failed，closed1\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == ProcessorFunction(_sock))
			{
				printf("failed，closed2\n");
				break;
			}
		}

		//printf("client，working\n");
		
		//Sleep(2000);//Window下
	}
	//7 关闭socket
	closesocket(_sock);

	//————
	//清除环境
	WSACleanup();
	printf("客户端退出\n");
	getchar();
	return 0;
}