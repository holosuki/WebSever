#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN //�ǵ�д��仰����Ȼ��������include�����⣻
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <vector>

//#pragma comment(lib,"ws2_32.lib") //����windows�����ã�ͨ�õ�ʵ���������ws2_32.lib��

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
	//���տͻ�������
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
	//����windows socket 2.x ������������2.2��
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
	//��������

	//--��socket API��������TCP�����
	//1 ����һ��SOCKET
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//AF_INET ipv4,SOCK_STREAM ��������TCPЭ��
	//2 bind �����ڽ��ܿͻ������ӵ�����˿�
	sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(4567);//host to net unsigned short,������ַת���������޷��Ŷ����͵�ַ��
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;// inet_addr("127.0.0.1"),��������;���� INADDR_ANY ���Ȿ����ַ��
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
	{
		printf("ERROR �����ڽ��ܿͻ������ӵ�����˿�ʧ��\n");
	}
	else
	{
		printf("�����ڽ��ܿͻ������ӵ�����˿ڳɹ�\n");
	}
	//3 listen ��������˿�
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("ERROR ��������˿�ʧ��\n");
	}
	else
	{
		printf("��������˿ڳɹ�\n");
	}
	char _recvbuf[128] = {};
	while (true)
	{
		//������ BSD socket select����
		fd_set fdRead;	//����������
		fd_set fdWrite;
		fd_set fdExp;
		FD_ZERO(&fdRead);	//���
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (int i = g_client.size() - 1; i >= 0; --i)
		{
			FD_SET(g_client[i], &fdRead);
		}

		timeval time = {0, 0};	//�����������߷������� 1�Ƕ�������ȴ�ʱ��

		//nfds ��һ������ֵ����ָfd_set�����е�������������Χ�����������������������+1
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &time);
		if (ret < 0)
		{
			printf("failed,close \n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) //�ж���û��
		{
			FD_CLR(_sock, &fdRead);
			//4 accept �ȴ����տͻ�������
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(sockaddr_in);
			SOCKET _clientSock = INVALID_SOCKET;
			_clientSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _clientSock)
			{
				printf("ERROR ���յ���Ч�ͻ���\n");
			}
			for (int n = g_client.size() - 1; n >= 0; --n)
			{
				NewUser userJoin;
				send(g_client[n], (const char*)&userJoin, sizeof(NewUser), 0);
			}
			printf("�¿ͻ��˼��룺SOCKET = %d IP = %s \n", _clientSock,inet_ntoa(clientAddr.sin_addr));
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
		//printf("����ʱ��ִ����������\n");
	}
	for (int k = 0; k < (int)g_client.size(); ++k)
	{
		closesocket(g_client[k]);
	}
	//6 �ر�socket
	closesocket(_sock);
	//��������
	//�������
	WSACleanup();
	printf("�������ر�\n");
	getchar();
	return 0;
}