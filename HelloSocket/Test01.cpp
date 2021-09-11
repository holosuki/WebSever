#define WIN32_LEAN_AND_MEAN //记得写这句话，不然下面两个include有问题；
#include<windows.h>
#include<winsock2.h>

//#pragma comment(lib,"ws2_32.lib") //仅在windows里适用，通用的实在属性里加ws2_32.lib；

int main()
{
	//启动windows socket 2.x 环境；下面是2.2；
	WORD ver = MAKEWORD(2,2);
	WSADATA dat;
	WSAStartup(ver,&dat);
	//————

	//————
	//清除环境
	WSACleanup();
	return 0;
}