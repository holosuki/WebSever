#ifdef _WIN32	//windowsϵͳ
	#define WIN32_LEAN_AND_MEAN //�ǵ�д��仰����Ȼ��������include�����⣻
	#include <windows.h>
	#include <winsock2.h>
	//#pragma comment(lib,"ws2_32.lib") //����windows�����ã�ͨ�õ�ʵ���������ws2_32.lib��
#else		//linux����MacOS
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include <stdio.h>
#include <thread>


//�����б�
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

//���ݰ�ͷ
//ǰ�ڲ�����short
struct DataHeaader
{
	short dataLength;	//���ݳ���
	short cmd;			//����
};

//���ݰ���
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
	//���տͻ�������
	int nlen = (int)recv(_csock, szRecv, sizeof(DataHeaader), 0);
	DataHeaader* _header = (DataHeaader*)szRecv;
	if (nlen <= 0)
	{
		printf("disconnect...\n");
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
	return 0;
}

bool g_bRun = true;
//�ͻ�������
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
			strcpy(log.userName, "zgr");
			strcpy(log.passWord, "23");
			send(_sock, (const char*)&log, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdbuf, "login")) {
			LogOut logout;
			strcpy(logout.userName, "�Ź���");
			send(_sock, (const char*)&logout, sizeof(LogOut), 0);
		}
		else {
			printf("commond error!\n");
		}
	}
}

int main()
{
#ifdef _WIN32
	//����windows socket 2.x ������������2.2��
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//��������
#endif

	//--��socket API��������TCP�ͻ���
	//1 ����һ��socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock)
	{
		printf("����socketʧ��\n");
	}
	else
	{
		printf("����socket�ɹ�\n");
	}
	//2 ���ӷ����� connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("192.168.31.179");//inet_addr("127.0.0.1");// INADDR_ANY;
#else 
	_sin.sin_addr.s_addr = inet_addr("192.168.132.1");	//����������ipv4��ַ
#endif
	int ret = connect(_sock,(sockaddr*)&_sin,sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret)
	{
		printf("���ӷ�����ʧ��\n");
	}
	else
	{
		printf("���ӷ������ɹ�\n");
	}
	
	//�߳�
	std::thread t1(cmdThread, _sock);
	t1.detach();	//�����̷߳��룬��Ϊ���߳����˳�

	while (g_bRun)
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);
		timeval time = { 1,0 };
#ifdef _WIN32
		int ret = select(_sock, &fdRead, NULL, NULL, &time);
#else
		int ret = select(_sock + 1, &fdRead, NULL, NULL, &time);
#endif
		if (ret < 0)
		{
			printf("failed��closed1\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead))
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == ProcessorFunction(_sock))
			{
				printf("failed��closed2\n");
				break;
			}
		}

		//printf("client��working\n");
		
		//Sleep(2000);//Window��
	}
	//7 �ر�socket
#ifdef _WIN32
	closesocket(_sock);

	//��������
	//�������
	WSACleanup();
#else 
	close(_sock);
#endif
	printf("�ͻ����˳�\n");
	getchar();
	return 0;
}