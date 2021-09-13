#include "EasyTcpClient.hpp"
#include <stdio.h>
#include <thread>

void cmdThread(EasyTcpClient* client) {
	while (true) {
		char cmdbuf[256] = {};
		scanf("%s", cmdbuf);
		if (0 == strcmp(cmdbuf, "exit")) {
			client->Close();
			printf("close\n");
			return;
		}
		else if (0 == strcmp(cmdbuf, "login")) {
			Login log;
			strcpy(log.userName, "zgr");
			strcpy(log.passWord, "23");
			client->SendData(&log);
		}
		else if (0 == strcmp(cmdbuf, "logout")) {
			LogOut logout;
			strcpy(logout.userName, "zgr");
			client->SendData(&logout);
		}
		else {
			printf("commond error!\n");
		}
	}
}

int main()
{
	EasyTcpClient client1;
	client1.Connect("127.0.0.1", 6000);

	EasyTcpClient client2;
	client2.Connect("192.168.31.179", 6001);

	//线程
	std::thread t1(cmdThread, &client1);
	t1.detach();	//与主线程分离，因为主线程先退出

	std::thread t2(cmdThread, &client2);
	t2.detach();

	while (client1.isRun() || client2.isRun())
	{
		client1.OnRun();
		client2.OnRun();
	}
	client1.Close();
	client2.Close();

	getchar();
	return 0;
}