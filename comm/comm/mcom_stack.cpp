#include <stdio.h>
#include "mcom_stack.h"
#include <winsock2.h>
#include <Windows.h>

#pragma comment (lib,"ws2_32.lib") //链接ws2_32.dll动态链接库
unsigned int state_map[DEV_TYP_NUM][DEV_ONE_TYPE_MAX_NUM];
dev_contacts_t dev_map_tab[MAX_DEVICE_NUMBER] = {
{0x00,0x000000,"192.168.1.25"},

};

unsigned char check_out_device_type(unsigned device_id)
{
	for (int i = 0; i < MAX_DEVICE_NUMBER; i++)
	{
		if (device_id == dev_map_tab[i].unique_id)
		{
			return  dev_map_tab[i].device_type;
		}
	}
	return 255;
}

char pro_send_pak(SOCKET sck, comm_frame_t frd)
{
	char res = 0;
	unsigned char dbuf[COM_PRO_ST_MAX_LEN];
	int len = 0;
	if (frd.cfh.pay_len >= COM_PRO_ST_MAX_LEN - 30)
	{
		exit(-1);
	}
	len = sizeof(frd.cfh);

	memcpy(dbuf,&frd.cfh,len);
	memcpy(&dbuf[len], frd.pdata, frd.cfh.pay_len);
	len += frd.cfh.pay_len;
	res = send(sck, (const char *)dbuf, len, 0);
	return res;
}

char send_command(unsigned target_device_id, cmd_val_t cmd_val, bool mode, SOCKET sck)
{
	comm_frame_t frd;
	char res = 0;
	cmd_val_t temp_cmd_val = cmd_val;
	frd.cfh.frame_header = 0x55AA;
	frd.cfh.su_device_type = 0x00;
	frd.cfh.tg_device_type = check_out_device_type(target_device_id);
	frd.cfh.op_code = 0x01;
	frd.cfh.su_unique_id = MASTER_DEVICE_ID;
 
	frd.cfh.pay_len = sizeof(temp_cmd_val);
	frd.pdata = (char*)&temp_cmd_val;
	res = pro_send_pak(sck, frd);
	return res;
}

int unpack_state(char* pdata, int len)
{
	comm_frame_t* pfrd;
	pfrd = (comm_frame_t*)pdata;
	int * state = (int*)pfrd->pdata;
	if (pfrd->cfh.op_code == 0x03)
	{
		if (pfrd->cfh.su_device_type < DEV_TYP_NUM)
		{
			if (pfrd->cfh.su_unique_id < DEV_ONE_TYPE_MAX_NUM)
			{
				state_map[pfrd->cfh.su_device_type][pfrd->cfh.su_unique_id] = *state;
			}
		}
	}
	else
	{
		return -1;
	}

	return *state;
}



/*share memory 4 state code */
/*
函数功能：检查IP地址
详细介绍：检查IP地址中的点是否是3个，以及每段IP的数值是否超过255
*/
int CheckIP(char *cIP)
{
	char IPAddress[128];//IP地址字符串
	char IPNumber[4];//IP地址中每组的数值
	int iSubIP = 0;//IP地址中4段之一
	int iDot = 0;//IP地址中'.'的个数
	int iResult = 0;
	int iIPResult = 1;
	int i;//循环控制变量

	memset(IPNumber, 0, 4);
	strncpy(IPAddress, cIP, 128);

	for (i = 0; i<128; i++)
	{
		if (IPAddress[i] == '.')
		{
			iDot++;
			iSubIP = 0;
			if (atoi(IPNumber)>255) //检查每段IP的数值是否超过255
				iIPResult = 0;
			memset(IPNumber, 0, 4);
		}
		else
		{
			IPNumber[iSubIP++] = IPAddress[i];
		}
		if (iDot == 3 && iIPResult != 0) //检查IP地址中的点是否是3个
			iResult = 1;
	}

	return iResult;
}

/*
函数功能：退出系统函数,并释放文件指针和ws2_32.lib动态链接库
*/
void ExitSystem()
{
	//if (server_fp != NULL)
	//	fclose(server_fp);
	//if (client_fp != NULL)
	//	fclose(client_fp);
	WSACleanup(); //释放初始化ws2_32.lib动态链接库所分配的资源
	exit(0);
}


/*
函数功能：创建服务端接收消息的线程
*/
DWORD WINAPI threadproServer(LPVOID lpParam) // LPVOID lpParameter为线程参数
{
	SOCKET hsock = (SOCKET)lpParam;
	char cRecvBuffer[1024]; //接收消息缓存,接收数据保存在cRecvBuff[]
	char cShowBuffer[1024]; //显示消息缓存
	int iRecvResult = 0;

	if (hsock != INVALID_SOCKET)
	{
		printf("start:\n");
	}

	while (1)
	{
		iRecvResult = recv(hsock, cRecvBuffer, 1024, 0);
		if (iRecvResult >= 0)
		{
			//cRecvBuffer[iRecvResult] = '\0'; //将cRecvBuff[]变为字符串
			//sprintf(cShowBuffer, "to me:%s\n", cRecvBuffer); //sprintf: 格式化的数据写入某个字符串中
			//printf("%s", cShowBuffer); //显示接收到的数据
			//fwrite(cShowBuffer, 1, strlen(cShowBuffer), server_fp); //将接收到的数据，写入到服务端文件中
			//fflush(server_fp); //刷新文件流 stream 的输出缓冲区 (文件指针本质也是一种流stream)
			unpack_state(cRecvBuffer, iRecvResult);
			if (strcmp("exit", cRecvBuffer) == 0)
			{
				ExitSystem(); //退出系统函数,并释放文件指针和ws2_32.lib动态链接库
				//退出系统
			}
		}
		else
		{
			ExitSystem(); //退出系统函数,并释放文件指针和ws2_32.lib动态链接库
		}
	}
	return 0;
}

/*
函数功能：创建点对点服务端
详细介绍：服务端监听客服端发来的连接请求，当有客户端发来连接请求时，启动接收消息的线程并进入发送消息的循环中
*/
void createServer()
{
	SOCKET    server_listenSocket; //服务端的监听套接字,socket()创建的，监听客户端是否发来连接请求
	SOCKET    server_communiSocket; //服务端的通信套接字,accept()返回的,与客户端进行通信
	struct sockaddr_in server_sockAddr; //包含服务端的本地接口和端口号的sockaddr_in结构体
	struct sockaddr_in client_sockAddr; //包含所连接客服端的接口和端口号的sockaddr_in结构体
	struct hostent *localHost; //包含本地主机的主机名和地址信息的hostent结构体指针

	int iPort = 4600; //设定为固定端口
	char* localIP; //本地主机的IP地址
	DWORD nThreadId = 0; //进程ID
	int iBindResult = -1; //绑定结果
	int ires;//发送的返回值
	int iWhileCount_bind = 10; //能够重新输入端口号绑定本地主机的机会次数
	int iWhileCount_listen = 10; //能够重新监听的机会次数
	char cWelcomBuffer[] = "Welcome to you\0"; //欢迎消息的字符串
	char cSendBuffer[1024];//发送消息缓存
	char cShowBuffer[1024];//接收消息缓存
	int len = sizeof(struct sockaddr);

	//server_fp = fopen("MessageServer.txt", "a");//打开记录消息的文件

	//创建一个服务端的本地连接套接字
	server_listenSocket = socket(AF_INET, SOCK_STREAM, 0); //TCP方式，故type选择SOCK_STREAM流式套接字


	//获取本地主机的IP地址
	localHost = gethostbyname(""); //获取包含本地主机的主机名和地址信息的hostent结构体指针
	localIP = "192.168.86.1";//inet_ntoa(*(struct in_addr *)*localHost->h_addr_list); //获取本地主机的IP地址

	//配置本地主机的网络地址信息
	server_sockAddr.sin_family = AF_INET; //设置地址家族                    
	server_sockAddr.sin_port = htons(iPort); //设置本地主机的端口号        
	server_sockAddr.sin_addr.S_un.S_addr = INADDR_ANY; //设置本地主机的IP地址

	//将套接字绑定在本地主机上
	iBindResult = bind(server_listenSocket, (struct sockaddr*)&server_sockAddr, sizeof(struct sockaddr));
	if (iBindResult != 0)
	{
		printf(" bind error %d \n", iBindResult);
		exit(0);
	}


	//重复监听
	while (1)
	{
		printf("start listen\n");
		listen(server_listenSocket, 0);//返回值判断单个监听是否超时

		server_communiSocket = accept(server_listenSocket, (struct sockaddr*)&client_sockAddr, &len);
		if (server_communiSocket != INVALID_SOCKET)
		{
			////有连接成功，发送欢迎信息
			//send(server_communiSocket, cWelcomBuffer, sizeof(cWelcomBuffer), 0);
			//启动接收消息的线程
			CreateThread(NULL, 0, threadproServer, (LPVOID)server_communiSocket, 0, &nThreadId);
//			break;
		}
		printf(".");
	}
}


int main()
{
	//初始化WSA
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		return 0;
	}
	createServer();
	printf("OK");
	return 0;
}

///https://www.cnblogs.com/linuxAndMcu/p/9787473.html

