#include "EasyTcpServer.hpp"

int main()
{
	EasyTcpServer server;
	server.initSocket();
	server.Bind("127.0.0.1", 6000);
	server.Listen(5);

	while (server.isRun()) server.OnRun();

	server.Close();

	printf("·şÎñÆ÷¹Ø±Õ\n");
	getchar();
	return 0;
}