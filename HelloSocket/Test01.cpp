#define WIN32_LEAN_AND_MEAN //�ǵ�д��仰����Ȼ��������include�����⣻
#include<windows.h>
#include<winsock2.h>

//#pragma comment(lib,"ws2_32.lib") //����windows�����ã�ͨ�õ�ʵ���������ws2_32.lib��

int main()
{
	//����windows socket 2.x ������������2.2��
	WORD ver = MAKEWORD(2,2);
	WSADATA dat;
	WSAStartup(ver,&dat);
	//��������

	//��������
	//�������
	WSACleanup();
	return 0;
}